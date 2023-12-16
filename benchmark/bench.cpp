#include <iostream>
#include <chrono>
#include <random>
#include <thread>
#include <benchmark/benchmark.h>
#include "thread_pool.hpp"
#include "lru_cache.hpp"
#include "lfu_cache.hpp"

std::random_device rd;

class TestFixture : public benchmark::Fixture {
 public:
  void SetUp(benchmark::State & state) override {
    fprintf(stdout, "[%zu] %s\n", uint64_t(obj_), __FUNCTION__);
  }

  void TearDown(benchmark::State & state) override {
    fprintf(stdout, "[%zu] %s\n", uint64_t(obj_), __FUNCTION__);
  }

 protected:
  uint64_t obj_{};
};

BENCHMARK_DEFINE_F(TestFixture, test) (benchmark::State & state) {
  for (auto _ : state) {
    std::this_thread::sleep_for(std::chrono::milliseconds (rd() % 1000));
    std::cout << std::this_thread::get_id() << ": " << uint64_t(obj_) << std::endl;
  }
}

//BENCHMARK_REGISTER_F(TestFixture, test)->Iterations(10)->Repetitions(1)->Threads(2);

class ThreadPoolFixture : public ::benchmark::Fixture {
 public:
  void SetUp(const ::benchmark::State & state) override {
    // std::this_thread::sleep_for(std::chrono::seconds(1));
    count_.fetch_add(1);
  }

  void TearDown(const ::benchmark::State & state) override {
    if (count_.fetch_sub(1) == 0) {
      pool_.stop(false);
    }
  }

 protected:
  ThreadPool pool_{1, [] (const string& id) { fprintf(stdout, "[%s] DONE\n", id.c_str()); }, 1};
  std::atomic<size_t> count_{0};
};

class LRUFixture : public ::benchmark::Fixture {
 public:
  void SetUp(::benchmark::State & state) override {
    //
  }

  void TearDown(::benchmark::State & state) override {
    //
  }

 protected:
  LRUCache<int64_t, int64_t> cache_{1024};
};

class LFUFixture : public benchmark::Fixture {
 public:
  void SetUp(::benchmark::State & state) override {
    //
  }

  void TearDown(::benchmark::State & state) override {
    //
  }

 protected:
  LFUCache<int64_t, int64_t> cache_{1024};
};

BENCHMARK_DEFINE_F(ThreadPoolFixture, case1) (::benchmark::State & state) {
  int64_t id = 0;
  for (auto _ : state) {
    pool_.enqueue(std::to_string(id++), [] (int64_t num) {return num;}, id);
  }
}

BENCHMARK_DEFINE_F(LRUFixture, case1) (::benchmark::State & state) {
  int64_t key = 0, val = 0;
  for (auto _ : state) {
    cache_.put(++key, val++);
    val = cache_.get(key);
  }
}

BENCHMARK_DEFINE_F(LFUFixture, case1) (::benchmark::State & state) {
  int64_t key = 0, val = 0;
  for (auto _ : state) {
    cache_.put(++key, val++);
    val = cache_.get(key);
  }
}

BENCHMARK_REGISTER_F(ThreadPoolFixture, case1)->Iterations(20000)->Repetitions(4)->Threads(2);
BENCHMARK_REGISTER_F(LRUFixture, case1)->Iterations(100000)->Repetitions(10)->Threads(1);
BENCHMARK_REGISTER_F(LFUFixture, case1)->Iterations(100000)->Repetitions(10)->Threads(1);

BENCHMARK_MAIN();
