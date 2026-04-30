#include "threadpool/thread_pool.h"

#include <chrono>
#include <iostream>
#include <thread>

int main() {
    threadpool::ThreadPool pool(4);

    for (int i = 0; i < 8; ++i) {
        pool.submit([i]() {
            std::cout << "task " << i
                      << " is running in thread "
                      << std::this_thread::get_id() << std::endl;

            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            std::cout << "task " << i << " finished" << std::endl;
        });
    }

    return 0;
}