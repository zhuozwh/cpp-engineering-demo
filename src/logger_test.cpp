#include "logger/logger.h"

#include <thread>
#include <vector>
#include <string>

void worker(int id) {
    for (int i = 0; i < 1000; ++i) {
        LOG_INFO("worker " + std::to_string(id) +
                 " log message " + std::to_string(i));
    }
}

int main() {
    logger::Logger::instance().set_level(logger::LogLevel::INFO);

    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back(worker, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    return 0;
}
