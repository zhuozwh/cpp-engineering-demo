cmake_minimum_required(VERSION 3.10)

project(cpp_service_framework LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(cpp_service_framework
    src/main.cpp
    src/logger/logger.cpp
    src/logger/log_level.cpp
    src/logger/log_sink.cpp
    src/logger/log_formatter.cpp
)

target_include_directories(cpp_service_framework PRIVATE
	${PROJECT_SOURCE_DIR}/include
)
