#include <iostream>
#include <string>
#include <chrono>
#include "thread_pool.hpp"
#include "lfu_cache.hpp"
#include "lru_cache.hpp"

using namespace std;
using namespace std::chrono;

#define sleep(sec) this_thread::sleep_for(chrono::seconds(sec));

template<typename Func, typename... Args>
void RunWithTimeCost(Func&& func, Args&&... args) {
  auto start_tp = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
  cout << "[begin at(ms)]: " << start_tp << endl;
  func(std::forward<Args &&>(args)...);
  auto end_tp = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
  cout << "[end at(ms)]: " << end_tp << endl;
  cout << "[used total(ms)]: " << end_tp - start_tp << endl << endl;
}

void demo_ThreadPool() {
  auto fcb = [](const string &id) {
    // not threadsafe
    fprintf(stdout, "task[%s] execute down.\n", id.c_str());
  };
  ThreadPool pool(4, fcb/*, 0*/);
  for (int i = 0; i < 10; ++i)
    pool.enqueue(std::to_string(i), [i] {
      sleep(1);
      fprintf(stdout, "task[%d] executing...\n", i);
      sleep(1);
    });
  fprintf(stdout, "func [%s] is abort to return\n", __func__);
}

void demo_LFUCache() {
  static const int MAX = 100000;
  LFUCache<int, int> lfu(MAX / 10);
  cout << "put" << endl;
  RunWithTimeCost([&lfu] {
    for (int i = 1; i <= MAX; ++i) {
      lfu.put(i, i * 5);
    }
  });

  cout << "get" << endl;
  RunWithTimeCost([&lfu] {
    for (int i = 1; i <= MAX; ++i) {
      // cout << "key=" << i << ", value=" << lfu.get(i) << endl;
      lfu.get(i);
    }
  });
}

void demo_LRUCache() {
  static const int MAX = 1000;
  LRUCache<int, int> lru(MAX / 10);
  cout << "put" << endl;
  RunWithTimeCost([&lru] {
    for (int i = 1; i <= MAX; ++i) {
      lru.put(i, i * 5);
    }
  });

  cout << "get" << endl;
  RunWithTimeCost([&lru] {
    for (int i = 1; i <= MAX; ++i) {
      // cout << "key=" << i << ", value=" << lru.get(i) << endl;
      lru.get(i);
    }
  });
}

int main(int argc, char** argv) {
//    demo_ThreadPool();
//    demo_LFUCache();
//    demo_LRUCache();

  ThreadPool pool(4);
  pool.stop(false);

  auto r = pool.enqueue("1", []{printf("1111\n");});
  r.get();

  return 0;
}
