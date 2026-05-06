#include "config/config.h"

#include <iostream>

int main() {
    config::Config cfg;

    // 从配置文件加载 key=value 配置。
    if (!cfg.load("config/server.conf")) {
        std::cerr << "failed to load config/server.conf" << std::endl;
        return 1;
    }

    // 正常读取 int 配置。
    std::cout << "server.port = "
              << cfg.get_int("server.port", 80)
              << std::endl;

    std::cout << "server.thread_num = "
              << cfg.get_int("server.thread_num", 1)
              << std::endl;

    // 正常读取 string 配置。
    std::cout << "log.level = "
              << cfg.get_string("log.level", "DEBUG")
              << std::endl;

    std::cout << "log.file = "
              << cfg.get_string("log.file", "stdout")
              << std::endl;

    // 正常读取 bool 配置。
    std::cout << "debug = "
              << std::boolalpha
              << cfg.get_bool("debug", false)
              << std::endl;

    // key 不存在时返回默认值。
    std::cout << "missing.value = "
              << cfg.get_string("missing.value", "default")
              << std::endl;

    return 0;
}