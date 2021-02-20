cmake_minimum_required(VERSION 3.8)
project(elfpatcher)

set(CMAKE_CXX_STANDARD 11)

add_executable(elfpatch
        elfpatcher/main.cpp
        elfpatcher/utils.cpp
        elfpatcher/utils.h
        elfpatcher/codegen.cpp
        elfpatcher/codegen.h
        elfpatcher/asm.h
        elfpatcher/elfpatcher.h
        elfpatcher/reader.cpp
        elfpatcher/reader.h
        elfpatcher/virtual.cpp
        elfpatcher/virtual.h)
TARGET_LINK_LIBRARIES(elfpatch elf)

include_directories(./include/)
