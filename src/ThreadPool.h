#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
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
  explicit ThreadPool(size_t n = std::thread::hardware_concurrency());
  ~ThreadPool();

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
  void worker_loop();
};
