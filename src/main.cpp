#include "logger/logger.h"
#include "logger/log_sink.h"
#include "logger/console_sink.h"
#include "logger/file_sink.h"

using namespace logger;

int main() {
    // 1. 设置日志级别
    Logger::instance().set_level(LogLevel::DEBUG);

    /* // 2. 设置输出 sink（v1.2 的关键）
    Logger::instance().set_sink(
        std::make_shared<ConsoleSink>()
    ); */

    Logger::instance().add_sink(
        std::make_shared<FileSink>("app.log")
    );

    // 3. 正常使用日志宏
    LOG_INFO("Server starting...");
    LOG_WARN("This is a warning");
    LOG_ERROR("Something went wrong _test_");

    return 0;
}
