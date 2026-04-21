#pragma once

#include <chrono>
#include <string>
#include <thread>

#include "log_level.h"

namespace logger {

// 日志消息实体：把一次日志需要的上下文信息集中起来，
// 后续 formatter / sink / async logger 都可以复用它。
struct LogMessage {
    LogLevel level;
    std::string message;

    // 记录日志调用位置，方便排查问题
    const char* file;
    int line;
    const char* func;

    // 记录打日志的时间和线程
    std::chrono::system_clock::time_point timestamp;
    std::thread::id thread_id;
};

}  // namespace logger