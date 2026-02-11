#pragma once

#include <string>

namespace logger {

class LogSink {
public:
    virtual ~LogSink() = default;

    // 写日志的统一接口
    virtual void write(const std::string& formatted_message) = 0; // virtual 是为了将来多态
};

} // namespace logger
