#include "logger/logger.h"
#include <iostream>
#include <mutex>

namespace logger {

Logger::Logger() : level_(LogLevel::DEBUG) {}

Logger::~Logger() = default;

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

void Logger::set_level(LogLevel level) {
    level_ = level;
}

void Logger::set_log_file(const std::string& filename) {
    // v0：暂不支持文件输出
    (void)filename;
}

void Logger::log(LogLevel level,
                 const char* file,
                 int line,
                 const char* func,
                 const std::string& message) {
    if (level < level_) {
        return;
    }

    std::cout
        << "[" << to_string(level) << "] "
        << file << ":" << line << " "
        << func << " | "
        << message
        << std::endl;
}

}

