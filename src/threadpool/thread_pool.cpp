#include "threadpool/thread_pool.h"

#include <stdexcept>
#include <utility>

namespace threadpool {

ThreadPool::ThreadPool(std::size_t thread_count) : stop_(false) {
    if (thread_count == 0) {
        throw std::invalid_argument("ThreadPool thread_count must be greater than 0");
    }

    workers_.reserve(thread_count);
    for (std::size_t i = 0; i < thread_count; ++i) {
        workers_.emplace_back(&ThreadPool::worker_loop, this);
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_ = true;
    }

    cv_.notify_all();

    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::submit(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (stop_) {
            throw std::runtime_error("submit on stopped ThreadPool");
        }

        tasks_.push(std::move(task));
    }

    cv_.notify_one();
}

void ThreadPool::worker_loop() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(mutex_);

            cv_.wait(lock, [this]() {
                return stop_ || !tasks_.empty();
            });

            if (stop_ && tasks_.empty()) {
                return;
            }

            task = std::move(tasks_.front());
            tasks_.pop();
        }

        task();
    }
}

}  // namespace threadpool