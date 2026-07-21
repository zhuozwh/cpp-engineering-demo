#pragma once

#include <string>

#include "log_message.h"

namespace logger {

// Formatter 只负责“把日志对象转成字符串”
// 不负责输出到控制台或文件
class LogFormatter {
public:
    LogFormatter() = default;
    ~LogFormatter() = default;

    std::string format(const LogMessage& msg) const;
};

}  // namespace logger
