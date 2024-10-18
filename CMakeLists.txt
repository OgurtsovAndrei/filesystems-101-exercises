cmake_minimum_required(VERSION 3.29)
project(FORK_filesystems_101_exercises)

set(CMAKE_CXX_STANDARD 20)

add_definitions(-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC)
add_definitions(-Wall -Wextra -Wshadow)
add_compile_options(-fsanitize=address -fsanitize=undefined)
add_link_options(-fsanitize=address -fsanitize=undefined)

#if(NOT CMAKE_BUILD_TYPE)
#    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
#endif()
#
#set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g")
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")

include_directories(00-ps)

add_executable(00
        00-ps/callbacks.c
        00-ps/main.c
        00-ps/solution.c
        00-ps/solution.h)
