#pragma once

namespace logger {

enum class LogLevel {
    DEBUG = 0,
    INFO,
    WARN,
    ERROR
};

const char* to_string(LogLevel level);

}
