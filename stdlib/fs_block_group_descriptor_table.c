//
// Created by andrei on 11/5/24.
//

#include "fs_block_group_descriptor_table.h"
#include "fs_superblock.h"
#include <stdio.h>
#include <unistd.h>
#include <asm-generic/errno.h>


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

int init_blockgroup_descriptor(int fd, fs_superblock *superblock,
                                      fs_blockgroup_descriptor *blockgroup_descriptor, uint32_t block_group_num) {
    __off64_t begining_of_blockgroup_descriptor;
    if (superblock->s_log_block_size_kbytes == 0) begining_of_blockgroup_descriptor = (1 << 11);
    else begining_of_blockgroup_descriptor = 1 << (10 + superblock->s_log_block_size_kbytes);
    // begining_of_blockgroup_descriptor += superblock->s_blocks_per_group * (1 << (10 + superblock->s_log_block_size_kbytes)) * block_group_num;
    begining_of_blockgroup_descriptor += sizeof(fs_blockgroup_descriptor) * block_group_num;
    if (pread(fd, blockgroup_descriptor, sizeof(fs_blockgroup_descriptor), begining_of_blockgroup_descriptor)
        != sizeof(fs_blockgroup_descriptor)) {
        return -EPROTO;
    }

    return 0;
}
