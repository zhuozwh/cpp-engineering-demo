cmake_minimum_required(VERSION 3.16)

project(cpp_service_framework VERSION 0.6.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 生成 compile_commands.json，方便 clangd 做跳转和补全
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)

# =========================
# 核心库：logger / threadpool / buffer / config / net
# =========================
add_library(cpp_service_framework
    src/logger/logger.cpp
    src/logger/log_level.cpp
    src/logger/log_sink.cpp
    src/logger/log_formatter.cpp
    src/logger/console_sink.cpp
    src/logger/file_sink.cpp
    src/threadpool/thread_pool.cpp
    src/buffer/buffer.cpp
    src/config/config.cpp
    src/net/socket.cpp
    src/net/channel.cpp
    src/net/epoll_poller.cpp
    src/net/event_loop.cpp
    src/net/acceptor.cpp
    src/net/tcp_connection.cpp
    src/net/tcp_server.cpp
)

target_include_directories(cpp_service_framework
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(cpp_service_framework
    PUBLIC
        Threads::Threads
)

if (MSVC)
    target_compile_options(cpp_service_framework PRIVATE /W4)
else()
    target_compile_options(cpp_service_framework PRIVATE -Wall -Wextra -Wpedantic)
endif()

# =========================
# examples
# =========================
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

add_executable(buffer_demo
    examples/buffer_demo.cpp
)

target_link_libraries(buffer_demo
    PRIVATE
        cpp_service_framework
)

add_executable(config_demo
    examples/config_demo.cpp
)

target_link_libraries(config_demo
    PRIVATE
        cpp_service_framework
)

add_executable(reactor_demo
    examples/reactor_demo.cpp
)

target_link_libraries(reactor_demo
    PRIVATE
        cpp_service_framework
)

add_executable(echo_server_demo
    examples/echo_server_demo.cpp
)

target_link_libraries(echo_server_demo
    PRIVATE
        cpp_service_framework
)
