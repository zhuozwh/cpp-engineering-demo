#include "logger/console_sink.h"

#include <iostream>

namespace logger {

void ConsoleSink::write(const std::string& formatted_message) {
    // sink 自己负责并发安全，避免多线程输出互相打断
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << formatted_message << std::endl;
}

}  // namespace logger