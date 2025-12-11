#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t n) {
  workers.reserve(n);
  for (size_t i = 0; i < n; ++i) {
    workers.emplace_back([this] { worker_loop(); });
  }
}

ThreadPool::~ThreadPool() {
  stop = true;
  cv.notify_all();
  // jthread auto-joins on destruction
}

void ThreadPool::worker_loop() {
  while (true) {
    std::function<void()> job;
    {
      std::unique_lock lk(mtx);
      cv.wait(lk, [this] { return stop || !tasks.empty(); });
      if (stop && tasks.empty()) {
        return;
      }
      job = std::move(tasks.front());
      tasks.pop();
    }
    job();
  }
}
