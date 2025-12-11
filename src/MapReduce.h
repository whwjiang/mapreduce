#pragma once
#include <algorithm>
#include <concepts>
#include <filesystem>
#include <format>
#include <fstream>
#include <future>
#include <mutex>
#include <vector>

#include <iostream>

#include "ThreadPool.h"

namespace mr {

// job definition
template <typename M, typename R> struct Job {
  M mapper;
  R reducer;
  const std::vector<std::filesystem::path> inputs;
  const std::filesystem::path output_dir;
  std::size_t num_workers = std::thread::hardware_concurrency();
  std::size_t num_mapper_tasks = std::thread::hardware_concurrency();
  std::size_t num_reduce_tasks = std::thread::hardware_concurrency();
};

// concept checks
template <typename F, typename Input, typename KOut, typename VOut>
concept MapperFn =
    requires(F f, Input in, std::function<void(KOut, VOut)> emit) {
      { f(in, emit) };
    };
template <typename F, typename KOut, typename VOut, typename ROut>
concept ReducerFn = requires(F f, KOut k, std::vector<VOut> vals,
                             std::function<void(KOut, ROut)> emit) {
  { f(k, vals, emit) };
};
template <typename T>
concept Hashable = requires(T a) {
  { std::hash<T>{}(a) } -> std::convertible_to<std::size_t>;
};
template <typename T>
concept StreamInsertable = requires(std::ostream &os, const T &a) {
  { os << a } -> std::same_as<std::ostream &>;
};
template <typename T>
concept HashableOrderedInsertable =
    Hashable<T> && std::totally_ordered<T> && StreamInsertable<T>;

// main entry point
template <typename Input, HashableOrderedInsertable KOut, typename VOut,
          StreamInsertable ROut, MapperFn<Input, KOut, VOut> M,
          ReducerFn<KOut, VOut, ROut> R>
void Run(const Job<M, R> &job) {
  ThreadPool pool(job.num_workers);
  // map the mapper output keys (intermediates) to a vector list of values
  std::vector<std::vector<std::pair<KOut, VOut>>> intermediates(
      job.num_reduce_tasks);
  std::mutex mtx; // one mutex for when mappers emit intermediates
  std::vector<std::future<void>> futures;
  futures.reserve(job.inputs.size());

  // map phase
  for (const auto &input : job.inputs) {
    futures.push_back(pool.enqueue([&, input] {
      const std::size_t batch_size = 100;
      std::vector<std::pair<KOut, VOut>> buffer;
      buffer.reserve(batch_size);

      auto emit = [&](const KOut &key, const VOut &value) {
        if (buffer.size() >= batch_size) {
          {
            std::lock_guard<std::mutex> lk(mtx);
            for (const auto &[k, v] : buffer) {
              std::size_t bucket = std::hash<KOut>{}(k) % job.num_reduce_tasks;
              intermediates[bucket].push_back({k, v});
            }
          }
          buffer.clear();
        }
        buffer.push_back({key, value});
      };

      job.mapper(input, emit);

      if (buffer.size() > 0) {
        {
          std::lock_guard<std::mutex> lk(mtx);
          for (const auto &[k, v] : buffer) {
            std::size_t bucket = std::hash<KOut>{}(k) % job.num_reduce_tasks;
            intermediates[bucket].push_back({k, v});
          }
        }
        buffer.clear();
      }
    }));
  }

  for (auto &f : futures) {
    f.get();
  }

  // reduce phase
  futures.clear();
  futures.reserve(job.num_reduce_tasks);

  for (std::size_t i = 0; i < job.num_reduce_tasks; ++i) {
    if (intermediates[i].empty()) {
      continue;
    }
    futures.push_back(pool.enqueue([&, i] {
      std::string outfile = std::format("{}/output-{}-of-{}.txt",
                                        job.output_dir.string(), i,
                                        job.num_reduce_tasks);
      std::ofstream fout(outfile, std::ios::app);

      auto emit = [&](const KOut &key, const ROut &value) {
        fout << key << ", " << value;
      };
      auto &pairs = intermediates[i];
      std::sort(pairs.begin(), pairs.end(),
                [](const auto &a, const auto &b) { return a.first < b.first; });

      std::vector<VOut> grouped;
      for (std::size_t idx = 0; idx < pairs.size();) {
        const auto &key = pairs[idx].first;
        grouped.clear();
        do {
          grouped.push_back(pairs[idx].second);
          ++idx;
        } while (idx < pairs.size() && pairs[idx].first == key);

        job.reducer(key, grouped, emit);
      }

      fout.close();
    }));
  }

  for (auto &f : futures) {
    f.get();
  }
}
} // namespace mr
