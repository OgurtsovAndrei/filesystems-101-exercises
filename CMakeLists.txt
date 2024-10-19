cmake_minimum_required(VERSION 3.29)
project(FORK_filesystems_101_exercises)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

add_definitions(-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC)


#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fsanitize=undefined -Wall -Wextra -Werror")
add_definitions(-Wall -Wextra -Wshadow)
add_definitions(-Werror)

add_compile_options(-fsanitize=address -fsanitize=undefined)
add_link_options(-fsanitize=address -fsanitize=undefined)


include_directories(00-ps)
add_executable(00
        00-ps/callbacks.c
        00-ps/main.c
        00-ps/solution.c
        00-ps/solution.h)

include_directories(01-lsof)
add_executable(01
        01-lsof/callbacks.c
        01-lsof/main.c
        01-lsof/solution.c
        01-lsof/solution.h)


add_definitions(-D_FILE_OFFSET_BITS=64 )
include_directories(02-fuse-helloworld)
add_executable(02
#        02-fuse-helloworld/solution1
        02-fuse-helloworld/solution.c
        02-fuse-helloworld/solution.h
        02-fuse-helloworld/main.c
)
target_link_libraries(02 fuse)