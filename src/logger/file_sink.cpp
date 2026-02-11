#include "logger/file_sink.h"

namespace logger {

FileSink::FileSink(const std::string& filename)
    : file_(filename, std::ios::app) {
}

void FileSink::write(const std::string& formatted_message) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (file_.is_open()) {
        file_ << formatted_message << std::endl;
    }
}

} // namespace logger
