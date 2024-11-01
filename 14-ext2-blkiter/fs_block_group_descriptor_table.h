//
// Created by andrei on 10/30/24.
//

#ifndef FS_BLOCK_GROUP_DESCRIPTOR_H
#define FS_BLOCK_GROUP_DESCRIPTOR_H

#include <stdint.h>

#include "fs_superblock.h"

#pragma pack(push, 1)

typedef struct {
    uint32_t address_of_block_usage_bitmap;       // Block address of block usage bitmap
    uint32_t address_of_inode_usage_bitmap;       // Block address of inode usage bitmap
    uint32_t address_of_inode_table;              // Starting block address of inode table
    uint16_t unallocated_blocks_counts;           // Number of unallocated blocks in group
    uint16_t unallocated_inodes_counts;           // Number of unallocated inodes in group
    uint16_t directories_in_group_count;          // Number of directories in group
} fs_blockgroup_descriptor;

int init_blockgroup_descriptor(int fd, fs_superblock *superblock, fs_blockgroup_descriptor *blockgroup_descriptor);

void print_block_group_descriptor_info(const fs_blockgroup_descriptor *descriptor) {
    printf("Block Group Descriptor Information:\n");
    printf("Block address of block usage bitmap: %u\n", descriptor->address_of_block_usage_bitmap);
    printf("Block address of inode usage bitmap: %u\n", descriptor->address_of_inode_usage_bitmap);
    printf("Starting block address of inode table: %u\n", descriptor->address_of_inode_table);
    printf("Number of unallocated blocks in group: %u\n", descriptor->unallocated_blocks_counts);
    printf("Number of unallocated inodes in group: %u\n", descriptor->unallocated_inodes_counts);
    printf("Number of directories in group: %u\n", descriptor->directories_in_group_count);
    printf("\n");
}

#pragma pack(pop)

#endif // FS_BLOCK_GROUP_DESCRIPTOR_H
