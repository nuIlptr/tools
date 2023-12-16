cmake_minimum_required(VERSION 3.25)
project(tools)

set(CMAKE_CXX_STANDARD 17)

include_directories(src)

set(CMAKE_BUILD_TYPE Release)

add_executable(
        demo
        test/demo.cc
        src/lfu_cache.hpp
        src/lru_cache.hpp
        src/thread_pool.hpp
)

add_executable(
        thread_pool_bench
        benchmark/bench.cpp
        src/thread_pool.hpp
)

target_link_libraries(
        thread_pool_bench
        benchmark
)
