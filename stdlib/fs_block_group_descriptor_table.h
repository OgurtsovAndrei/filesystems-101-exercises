//
// Created by andrei on 10/30/24.
//

#ifndef FS_BLOCK_GROUP_DESCRIPTOR_H
#define FS_BLOCK_GROUP_DESCRIPTOR_H

#include <stdint.h>

#include "fs_superblock.h"

#pragma pack(push, 1)

typedef struct {
    uint32_t address_of_block_usage_bitmap; // Block address of block usage bitmap
    uint32_t address_of_inode_usage_bitmap; // Block address of inode usage bitmap
    uint32_t address_of_inode_table; // Starting block address of inode table
    uint16_t unallocated_blocks_counts; // Number of unallocated blocks in group
    uint16_t unallocated_inodes_counts; // Number of unallocated inodes in group
    uint16_t directories_in_group_count; // Number of directories in group
} fs_blockgroup_descriptor;

#pragma pack(pop)

int init_blockgroup_descriptor(int fd, fs_superblock *superblock, fs_blockgroup_descriptor *blockgroup_descriptor, uint32_t group_offset);

void print_block_group_descriptor_info(const fs_blockgroup_descriptor *descriptor);


#endif // FS_BLOCK_GROUP_DESCRIPTOR_H
