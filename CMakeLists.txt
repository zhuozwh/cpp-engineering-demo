cmake_minimum_required(VERSION 3.10)

project(cpp_service_framework LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 生成 compile_commands.json，方便 clangd 做跳转和补全
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# =========================
# logger 模块：单独做成一个库
# =========================
add_library(logger_lib
    src/logger/logger.cpp
    src/logger/log_level.cpp
    src/logger/log_sink.cpp
    src/logger/log_formatter.cpp
    src/logger/console_sink.cpp
    src/logger/file_sink.cpp
)

target_include_directories(logger_lib PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

# =========================
# 主程序
# =========================
add_executable(cpp_service_framework
    src/main.cpp
)

target_link_libraries(cpp_service_framework PRIVATE
    logger_lib
)

# =========================
# Logger demo / test
# =========================
add_executable(logger_test
    src/logger_test.cpp
)

target_link_libraries(logger_test PRIVATE
    logger_lib
)