cmake_minimum_required(VERSION 3.16)

project(cpp_service_framework VERSION 0.4.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 生成 compile_commands.json，方便 clangd 做跳转和补全
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# =========================
# logger 模块：单独做成一个库
# =========================
add_library(cpp_service_framework
    src/logger/logger.cpp
    src/logger/log_level.cpp
    src/logger/log_sink.cpp
    src/logger/log_formatter.cpp
    src/logger/console_sink.cpp
    src/logger/file_sink.cpp
    src/threadpool/thread_pool.cpp
)

target_include_directories(cpp_service_framework
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

find_package(Threads REQUIRED)
target_link_libraries(cpp_service_framework
    PUBLIC
        Threads::Threads
)

# ============examples=============
add_executable(logger_test
    examples/logger_test.cpp
)

target_link_libraries(logger_test
    PRIVATE
        cpp_service_framework
)

add_executable(threadpool_demo
    examples/threadpool_demo.cpp
)

target_link_libraries(threadpool_demo
    PRIVATE
        cpp_service_framework
)