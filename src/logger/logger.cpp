#include "logger/logger.h"

#include <iostream>     // std::cout
#include <mutex>        // std::mutex, std::lock_guard
#include <sstream>      // std::ostringstream
#include<iomanip>
#include<chrono>

namespace {

std::string current_time() {
    using namespace std::chrono;

    auto now = system_clock::now();
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    std::time_t t = system_clock::to_time_t(now);
    std::tm tm_time;

    localtime_r(&t, &tm_time);  // Linux 线程安全版本

    std::ostringstream oss;
    oss << std::put_time(&tm_time, "%Y-%m-%d %H:%M:%S")
        << "." << std::setw(3) << std::setfill('0') << ms.count();

    return oss.str();
}

} // anonymous namespace

namespace logger {

// 获取全局唯一 Logger 实例
// 使用 Meyers Singleton：
// - C++11 保证线程安全
// - 懒加载
Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

// 构造函数：初始化日志级别
// 默认打印 DEBUG 及以上
Logger::Logger()
    : level_(LogLevel::DEBUG) {}

// 析构函数暂时为空
// v1 阶段没有资源需要释放
Logger::~Logger() = default;

// 设置日志级别
// 注意：这里也需要加锁
// 因为 level_ 会被多个线程同时读写
void Logger::set_level(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;
}

// v1 阶段不实现文件输出
// 这个接口是为 v2 预留的
// void Logger::set_log_file(const std::string& /*filename*/) {
   // // TODO(v2): file sink
//}

// Logger 的核心函数
void Logger::log(LogLevel level,
                 const char* file,
                 int line,
                 const char* func,
                 const std::string& message) {
    // 使用 RAII 风格的加锁
    // 确保一次 log 调用是原子操作

    // 日志级别过滤
    if (level < level_) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    // 使用 stringstream 统一拼接
    // 避免多次 << 造成输出交叉
    std::ostringstream oss;

    oss << "[" << to_string(level) << "] "
        << "[" << current_time() << "]"
	<< file << ":" << line << " "
        << func << " - "
        << message << std::endl; 
    
    // v1：输出到标准输出
    std::cout << oss.str();
}

} // namespace logger
