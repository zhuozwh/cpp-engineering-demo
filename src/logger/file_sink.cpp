#include "logger/file_sink.h"

#include <stdexcept>

namespace logger {

FileSink::FileSink(const std::string& filename)
    : file_(filename, std::ios::app) {
    if (!file_.is_open()) {
        throw std::runtime_error("Failed to open log file: " + filename);
    }
}

void FileSink::write(const std::string& formatted_message) {
    // 文件写入也要加锁，否则多线程下容易交叉写乱
    std::lock_guard<std::mutex> lock(mutex_);
    file_ << formatted_message << std::endl;
    file_.flush(); // 确保日志及时写入文件，避免崩溃时丢失（缓冲区日志刷进去）
}

}  // namespace logger