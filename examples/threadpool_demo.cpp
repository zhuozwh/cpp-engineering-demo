#include "threadpool/thread_pool.h"

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <future>
#include <mutex>

int main() {
    threadpool::ThreadPool pool(4);


    std::mutex cout_mutex;
    std::vector<std::future<int>> results;

    for (int i = 0; i < 8; ++i) {
        results.emplace_back(pool.submit([i, &cout_mutex]() {
            {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "task " << i
                      << " is running in thread "
                      << std::this_thread::get_id() << std::endl;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "task " << i << " finished" << std::endl;
            }

            return i * i;
        }));
    }

    for (auto& result : results) {
        int value = result.get();

        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "result = " << value << std::endl;
    }

    return 0;
}