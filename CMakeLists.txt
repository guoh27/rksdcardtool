cmake_minimum_required(VERSION 3.10)
project(rksdcardtool)
set(CMAKE_CXX_STANDARD 11)

add_executable(rksdcardtool
    RKSdcardTool.cpp
    RKBoot.cpp
    RKImage.cpp
    crc.cpp
)

