cmake_minimum_required(VERSION 3.10)
project(rksdcardtool VERSION 1.0)
set(CMAKE_CXX_STANDARD 11)

include(GNUInstallDirs)

add_executable(rksdcardtool
    RKSdcardTool.cpp
    RKBoot.cpp
    RKImage.cpp
    crc.cpp
)

install(TARGETS rksdcardtool
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})


