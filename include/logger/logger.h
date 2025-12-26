#pragma once

#include <string>
#include "log_level.h"

namespace logger {

class Logger {
public:
    static Logger& instance();

    void set_level(LogLevel level);
    void set_log_file(const std::string& filename);

    void log(LogLevel level,
             const char* file,
             int line,
             const char* func,
             const std::string& message);

private:
    Logger();
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    LogLevel level_; 
};

} // namespace logger

// 日志宏

#define LOG_DEBUG(msg) \
    logger::Logger::instance().log(logger::LogLevel::DEBUG, __FILE__, __LINE__, __func__, msg)

#define LOG_INFO(msg) \
    logger::Logger::instance().log(logger::LogLevel::INFO, __FILE__, __LINE__, __func__, msg)

#define LOG_WARN(msg) \
    logger::Logger::instance().log(logger::LogLevel::WARN, __FILE__, __LINE__, __func__, msg)

#define LOG_ERROR(msg) \
    logger::Logger::instance().log(logger::LogLevel::ERROR, __FILE__, __LINE__, __func__, msg)


