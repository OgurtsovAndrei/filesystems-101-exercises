//
// Created by andrei on 10/30/24.
//

#include "fs_superblock.h"

#include <stdio.h>
#include <unistd.h>
#include <asm-generic/errno.h>

int init_superblock(int fd, fs_superblock *superblock) {
    if (pread(fd, superblock, sizeof(fs_superblock), SUPERBLOCK_OFFSET) != sizeof(fs_superblock)) return -EPROTO;
    if (superblock->s_magic != 0xEF53) return -EPROTO;
    return 0;
}

void print_superblock_info(const fs_superblock *superblock) {
    printf("Superblock Information:\n");
    printf("Total number of inodes: %u\n", superblock->s_inodes_count);
    printf("Total number of blocks: %u\n", superblock->s_blocks_count);
    printf("Number of reserved blocks for superuser: %u\n", superblock->s_us_reserved_blocks_count);
    printf("Number of unallocated blocks: %u\n", superblock->s_free_blocks_count);
    printf("Number of unallocated inodes: %u\n", superblock->s_free_inodes_count);
    printf("Block number of the first data block: %u\n", superblock->s_first_data_block);
    printf("Block size (logarithmic): %u\n", superblock->s_log_block_size_kbytes);
    printf("Fragment size (logarithmic): %u\n", superblock->s_log_frag_size_kbytes);
    printf("Number of blocks per group: %u\n", superblock->s_blocks_per_group);
    printf("Number of inodes per group: %u\n", superblock->s_inodes_per_group);
    printf("Last mount time: %u\n", superblock->s_mtime);
    printf("Last write time: %u\n", superblock->s_wtime);
    printf("Magic number: 0x%04X\n", superblock->s_magic);
    printf("Inode Size: %u\n", superblock->s_inode_size);
    printf("\n");
}