#include "logger/logger.h"
#include "logger/file_sink.h"

#include <memory>
#include <string>
#include <thread>
#include <vector>

void worker(int id) {
    for (int i = 0; i < 1000; ++i) {
        LOG_INFO("worker " + std::to_string(id) +
                 " log message " + std::to_string(i));
    }
}

int main() {
    logger::Logger::instance().set_level(logger::LogLevel::INFO);

    // 在默认控制台输出基础上，再额外挂一个文件输出
    logger::Logger::instance().add_sink(
        std::make_shared<logger::FileSink>("logs/app0420.log"));

    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back(worker, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    LOG_WARN("logger v0.3.0 demo finished");
    return 0;
}