cmake_minimum_required(VERSION 2.6)
project(sdcard_tool)
set(CMAKE_CXX_STANDARD 11)

add_executable(sdcard_tool
    sdcard_tool.cpp
    RKBoot.cpp
    RKImage.cpp
    crc.cpp
)

