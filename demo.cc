#include <iostream>
#include <string>
#include <chrono>
#include "ThreadPool.hpp"

using namespace std;

#define sleep(sec) this_thread::sleep_for(chrono::seconds(sec));

void demo_ThreadPool()
{
    auto fcb = [] (const string& id) {
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

int main(int argc, char** argv)
{
    demo_ThreadPool();
    return 0;
}
