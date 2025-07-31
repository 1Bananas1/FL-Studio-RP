cmake_minimum_required(VERSION 3.10)
project(FLRP)

set(CMAKE_CXX_STANDARD 17)

add_executable(FLRP
    src/config.cpp
    src/monitor.cpp
    src/parser.cpp
    src/discord_rp.cpp
    src/main.cpp
)

target_include_directories(FLRP PRIVATE src/ lib/)