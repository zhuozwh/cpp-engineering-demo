#include "logger/logger.h"

#include <chrono>
#include <thread>
#include <utility>

#include "logger/log_message.h"

namespace logger {

Logger& Logger::instance() {
    static Logger instance; // 创建了一个“只会初始化一次，并且会一直存在”的Logger对象。(单例)
    return instance;
}

Logger::Logger()
    : level_(LogLevel::INFO) {
    // 默认挂一个控制台输出，避免用户未配置 sink 时日志直接丢失
    sinks_.push_back(std::make_shared<ConsoleSink>());
}

Logger::~Logger() = default;

void Logger::set_level(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;
}

void Logger::add_sink(std::shared_ptr<LogSink> sink) {
    if (!sink) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    sinks_.push_back(std::move(sink));
}

void Logger::clear_sinks() {
    std::lock_guard<std::mutex> lock(mutex_);
    sinks_.clear();
}

void Logger::log(LogLevel level,
                 const char* file,
                 int line,
                 const char* func,
                 const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);

    // 日志级别过滤：低于当前 Logger 级别的日志直接丢弃
    if (level < level_) {
        return;
    }

    // 理论上不会为空，但这里兜底，保证最少还能输出到控制台
    if (sinks_.empty()) {
        sinks_.push_back(std::make_shared<ConsoleSink>());
    }

    // 先组装日志对象，再统一交给 formatter 处理
    LogMessage log_message{
        level,
        message,
        file,
        line,
        func,
        std::chrono::system_clock::now(),
        std::this_thread::get_id()
    };

    const std::string formatted = formatter_.format(log_message);

    // 一条日志可以同时写到多个输出目标
    for (const auto& sink : sinks_) {
        if (sink) {
            sink->write(formatted);
        }
    }
}

}  // namespace logger