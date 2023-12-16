#pragma once

#include <thread>
#include <utility>
#include <vector>
#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <string>
#include <unordered_map>

using std::thread;
using std::vector;
using std::queue;
using std::shared_ptr;
using std::mutex;
using std::condition_variable;
using std::function;
using std::future;
using std::packaged_task;
using std::atomic;
using std::string;
using std::unordered_map;

class ThreadPool final {
  struct TaskInfo {
    string id;
    function<void()> callback;

    explicit TaskInfo(string id = "", function<void()> &&cb = nullptr)
        : id(std::move(id)), callback(std::move(cb)) {
    }
  };

  using fn_fin_t = void(*)(const string &);

 public:
  explicit ThreadPool(size_t maxWorkers, fn_fin_t fcb = nullptr, size_t maxJobs = 0)
      : finishCallback_(fcb), maxQueuingJobs_(maxJobs), stopped_(false) {
    maxWorkers = (maxWorkers == 0 ? std::thread::hardware_concurrency() : maxWorkers);
    workers_.reserve(maxWorkers);
    for (size_t i = 0; i < maxWorkers; ++i) {
      workers_.emplace_back([this] {
        for (;;) {
          TaskInfo job;

          {
            // fetch job from jobs_
            std::unique_lock<mutex> lock(this->lck_);
            this->empty_.wait(lock, [this] { return this->stopped_ || !this->jobs_.empty(); });
            if (this->stopped_ && this->jobs_.empty()) {
              break;
            }
            job = std::move(this->jobs_.front());
            this->jobs_.pop();
          }
          full_.notify_one();

          // execute job
          (job.callback)();
          // finish callback
          if (this->finishCallback_) {
            (this->finishCallback_)(job.id);
          }
        }
      });
    }
  }

  ~ThreadPool() {
    stop(false);
  }

  template<typename F, typename... Args>
  auto enqueue(const string &id, F &&f, Args &&... args) -> future<decltype(f(args...))>;

  void stop(bool abort) {
    {
      std::lock_guard<mutex> lock(lck_);
      if (stopped_) return;
      if (abort) {
        decltype(jobs_)().swap(jobs_);
      }
      stopped_ = true;
    }

    empty_.notify_all();
    for (auto & worker : workers_) {
      worker.join();
    }
  }

 private:
  vector<thread> workers_;
  fn_fin_t finishCallback_;
  queue<TaskInfo> jobs_;
  mutable mutex lck_;
  mutable condition_variable empty_;
  mutable condition_variable full_;
  size_t maxQueuingJobs_;         // maximum of queue, 0 means queuing is not allowed
  bool stopped_;
};

template<typename F, typename... Args>
auto ThreadPool::enqueue(const string& id, F&& f, Args&&... args) -> future<decltype(f(args...))> {
  using result_type = decltype(f(args...));
  auto task = std::make_shared<packaged_task<result_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));

  auto ret = task->get_future();
  do {
    std::unique_lock<mutex> lock(lck_);
    full_.wait(lock,
               [this] { return this->stopped_ || this->jobs_.size() < 1 + this->maxQueuingJobs_; });
    if (stopped_) {
      break;
    }
    jobs_.emplace(id, [task] { (*task)(); });
  } while (false);

  empty_.notify_one();
  return ret;
}
