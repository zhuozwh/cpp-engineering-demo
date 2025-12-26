#include "logger/log_level.h"

namespace logger {

const char* to_string(LogLevel level) {
    switch (level) {
    case LogLevel::DEBUG: return "DEBUG";
    case LogLevel::INFO:  return "INFO";
    case LogLevel::WARN:  return "WARN";
    case LogLevel::ERROR: return "ERROR";
    default:              return "UNKNOWN";
    }
}

}

