cmake_minimum_required(VERSION 3.10)
project(TestMonitor)

set(CMAKE_CXX_STANDARD 17)

add_executable(TestMonitor
    src/main2.cpp
    src/config.cpp
    src/monitor.cpp
)

target_include_directories(TestMonitor PRIVATE src/ lib/)