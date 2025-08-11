cmake_minimum_required(VERSION 3.10)
project(FLRP)

set(CMAKE_CXX_STANDARD 17)

add_executable(FLRP WIN32
    src/config.cpp
    src/monitor.cpp
    src/parser.cpp
    src/discord_rp.cpp
    src/app_state.cpp
    src/tray.cpp
    src/main.cpp
    public/app.rc
)

target_include_directories(FLRP PRIVATE src/ lib/)