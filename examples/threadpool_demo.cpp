#include "threadpool/thread_pool.h"

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <future>

int main() {
    threadpool::ThreadPool pool(4);

    std::vector<std::future<int>> results;

    for (int i = 0; i < 8; ++i) {
        results.emplace_back(pool.submit([i]() {
            std::cout << "task " << i
                      << " is running in thread "
                      << std::this_thread::get_id() << std::endl;

            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            
            std::cout << "task " << i << " finished" << std::endl;

            return i * i;
        }));
    }

    for (auto& result : results) {
        std::cout << "result = " << result.get() << std::endl;
    }

    return 0;
}