#include "logger/console_sink.h"
#include <iostream>

namespace logger {

void ConsoleSink::write(const std::string& formatted_message) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << formatted_message << std::endl;
}

} // namespace logger
