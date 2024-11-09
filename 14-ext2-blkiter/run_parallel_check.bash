#!/bin/bash

IMAGE_FILE="./../../NUP/FileSystems/ext2_large_image.img"
START_INODE=10
END_INODE=20000
NUM_THREADS=10

seq $START_INODE $END_INODE | parallel -j $NUM_THREADS ./14-ext2-blkiter/check_block.sh {} "$IMAGE_FILE"
