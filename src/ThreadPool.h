#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
  std::vector<std::jthread> workers;
  std::queue<std::function<void()>> tasks;
  std::condition_variable cv;
  std::mutex mtx;
  std::atomic<bool> stop{false};

public:
  explicit ThreadPool(size_t n = std::thread::hardware_concurrency()) {
    workers.reserve(n);
    for (size_t i = 0; i < n; ++i)
      workers.emplace_back([this] { worker_loop(); });
  }

  ~ThreadPool() {
    stop = true;
    cv.notify_all();
    // jthread auto-joins on destruction
  }

  template <typename F, typename... Args>
  auto enqueue(F &&f, Args &&...args)
      -> std::future<std::invoke_result_t<F, Args...>> {
    using R = std::invoke_result_t<F, Args...>;
    auto task = std::make_shared<std::packaged_task<R()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<R> fut = task->get_future();
    {
      std::unique_lock lk(mtx);
      tasks.emplace([task] { (*task)(); });
    }
    cv.notify_one();
    return fut;
  }

private:
  void worker_loop() {
    while (true) {
      std::function<void()> job;
      {
        std::unique_lock lk(mtx);
        cv.wait(lk, [this] { return stop || !tasks.empty(); });
        if (stop && tasks.empty())
          return;
        job = std::move(tasks.front());
        tasks.pop();
      }
      job();
    }
  }
};