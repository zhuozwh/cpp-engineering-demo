#include "logger/log_formatter.h"

#include <ctime>
#include <iomanip>
#include <sstream>

namespace logger {

const char* LogFormatter::level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARN:
            return "WARN";
        case LogLevel::ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

std::string LogFormatter::format(const LogMessage& msg) const {
    std::ostringstream oss;

    auto time_t_value = std::chrono::system_clock::to_time_t(msg.timestamp);

    std::tm tm_value{};
#ifdef _WIN32
    localtime_s(&tm_value, &time_t_value);
#else
    localtime_r(&time_t_value, &tm_value);
#endif

    // 统一把一条日志格式化成单行字符串，后面 sink 直接写出即可
    oss << "["
        << std::put_time(&tm_value, "%Y-%m-%d %H:%M:%S")
        << "]";

    oss << "["
        << level_to_string(msg.level)
        << "]";

    oss << "[tid:"
        << msg.thread_id
        << "]";

    oss << "["
        << msg.file
        << ":"
        << msg.line
        << "]";

    oss << "["
        << msg.func
        << "] ";

    oss << msg.message;

    return oss.str();
}

}  // namespace logger