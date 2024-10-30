cmake_minimum_required(VERSION 3.29)
project(FORK_filesystems_101_exercises)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

add_definitions(-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC)


#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fsanitize=undefined -Wall -Wextra -Werror")
add_definitions(-Wall -Wextra -Wshadow)
#add_definitions(-Werror)

add_compile_options(-fsanitize=address -fsanitize=undefined)
add_link_options(-fsanitize=address -fsanitize=undefined)
include_directories(stdlib)

#include_directories(00-ps)
add_executable(00
        00-ps/callbacks.c
        00-ps/main.c
        00-ps/solution.c
        00-ps/solution.h)

add_executable(01
        01-lsof/callbacks.c
        01-lsof/main.c
        01-lsof/solution.c
        01-lsof/solution.h)


add_definitions(-D_FILE_OFFSET_BITS=64)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FUSE3 REQUIRED fuse3)
include_directories(${FUSE3_INCLUDE_DIRS})

#include_directories(02-fuse-helloworld)
add_executable(02
        #        02-fuse-helloworld/solution1
        02-fuse-helloworld/solution.c
        02-fuse-helloworld/solution.h
        02-fuse-helloworld/main.c
)
target_link_libraries(02 ${FUSE3_LIBRARIES})

#include_directories(13-realpath)
add_executable(13
        13-realpath/callbacks.c
        13-realpath/main.c
        13-realpath/solution.c
        13-realpath/solution.h
)

#include_directories(03-io_uring)
add_executable(03
        03-io_uring/main.c
        03-io_uring/solution.c
        03-io_uring/solution.h
        stdlib/fs_malloc.c
)
target_link_libraries(03 uring)

#add_executable(03-cat
#        03-io_uring/cat
#)

add_executable(14
        14-ext2-blkiter/main.c
        14-ext2-blkiter/solution.c
        14-ext2-blkiter/solution.h
        14-ext2-blkiter/main.c
        14-ext2-blkiter/solution.h
        stdlib/fs_malloc.c
)

add_executable(14-read_bytes
        14-ext2-blkiter/read_bytes_ex.c
        14-ext2-blkiter/solution.h
        stdlib/fs_malloc.c
)

# curl -s -X POST http://185.106.102.104:9091/submit/OgurtsovAndrei/03-io_uring
# curl -s -X GET http://185.106.102.104:9091/results/OgurtsovAndrei/03-io_uring/last | jq