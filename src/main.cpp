#include "logger/logger.h"

int main() {
    LOG_INFO("Server starting...");
    LOG_WARN("This is a warning");
    LOG_ERROR("Something went wrong");

    return 0;
}

/*
#include<thread>
#include<vector>
#include<iostream>
#include<string>

#include"logger/logger.h"

int main() {
    logger::Logger::instance().set_level(logger::LogLevel::DEBUG);

    const int thread_count = 8;
    const int log_per_thread = 5000;

    std::vector<std::thread> threads;

    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back([i]() {
            for (int j = 0; j < log_per_thread; ++j) {
                LOG_INFO("thread " + std::to_string(i) +
                         " log " + std::to_string(j));
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    return 0;
} */
