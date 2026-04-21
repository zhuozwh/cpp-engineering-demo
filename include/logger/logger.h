#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "console_sink.h"
#include "log_formatter.h"
#include "log_level.h"
#include "log_sink.h"

namespace logger {

class Logger {
public:
    static Logger& instance();

    void set_level(LogLevel level);

    // 支持注册多个输出目标，例如控制台 + 文件
    void add_sink(std::shared_ptr<LogSink> sink);
    void clear_sinks();

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
    std::mutex mutex_;

    // 改成多 sink，便于以后扩展不同输出位置
    std::vector<std::shared_ptr<LogSink>> sinks_;

    // formatter 负责统一日志格式
    LogFormatter formatter_;
};

}  // namespace logger

#define LOG_DEBUG(msg) \
    logger::Logger::instance().log(logger::LogLevel::DEBUG, __FILE__, __LINE__, __func__, msg)

#define LOG_INFO(msg) \
    logger::Logger::instance().log(logger::LogLevel::INFO, __FILE__, __LINE__, __func__, msg)

#define LOG_WARN(msg) \
    logger::Logger::instance().log(logger::LogLevel::WARN, __FILE__, __LINE__, __func__, msg)

#define LOG_ERROR(msg) \
    logger::Logger::instance().log(logger::LogLevel::ERROR, __FILE__, __LINE__, __func__, msg)