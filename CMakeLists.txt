cmake_minimum_required(VERSION 3.29)
project(FORK_filesystems_101_exercises)

set(CMAKE_CXX_STANDARD 14)

include_directories(00-ps)

add_executable(00
        00-ps/callbacks.c
        00-ps/main.c
        00-ps/solution.c
        00-ps/solution.h)
