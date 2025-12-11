#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "MapReduce.h"

namespace fs = std::filesystem;

fs::path input_dir = "/home/whjiang/src/projects/mapreduce/examples/input";
fs::path output_dir = "/home/whjiang/src/projects/mapreduce/examples/output";

std::vector<fs::path> get_files_in_directory(const fs::path &path) {
  std::vector<fs::path> files;
  try {
    for (const auto &entry : fs::directory_iterator(path)) {
      if (fs::is_regular_file(entry.status())) {
        files.push_back(entry.path());
      }
    }
  } catch (const fs::filesystem_error &e) {
    std::cerr << "Filesystem error: " << e.what() << std::endl;
  }
  return files;
}

int main() {
  auto mapper = [](fs::path input, std::function<void(std::string, int)> emit) {
    std::ifstream fin(input);
    std::string line;
    while (getline(fin, line)) {
      std::istringstream iss(line);
      std::string token;
      while (getline(iss, token, ' ')) {
        emit(token, 1);
      }
    }
  };

  auto reducer = [](std::string key, std::vector<int> vals,
                    std::function<void(std::string, int)> emit) {
    int sum = 0;
    for (const auto &val : vals) {
      sum += val;
    }
    emit(key, sum);
  };

  std::vector<fs::path> inputs = get_files_in_directory(input_dir);
  std::size_t num_workers = std::thread::hardware_concurrency();
  std::size_t num_mapper_tasks = inputs.size();
  std::size_t num_reduce_tasks = 20; // arbitrary
  mr::Job<decltype(mapper), decltype(reducer)> job{
      mapper,      reducer,          inputs,          output_dir,
      num_workers, num_mapper_tasks, num_reduce_tasks};

  mr::Run<fs::path, std::string, int, int>(job);
  return 0;
}
