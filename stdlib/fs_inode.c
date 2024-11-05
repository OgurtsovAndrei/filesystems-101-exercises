//
// Created by andrei on 10/30/24.
//


#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

#include "fs_block_group_descriptor_table.h"
#include "fs_superblock.h"
#include "fs_inode.h"

#include "fs_malloc.h"

int init_inode(int fd, fs_superblock *superblock, fs_blockgroup_descriptor *blockgroup_descriptor, int inode_number,
               fs_inode **inode_ptr) {
    if ((uint32_t) inode_number >= superblock->s_inodes_count) return -EINVAL;

    uint32_t local_inode_index = (inode_number - 1) % superblock->s_inodes_per_group;
    uint16_t inode_size = superblock->s_inode_size;
    __off64_t block_size = 1 << (10 + superblock->s_log_block_size_kbytes);
    __off64_t inode_block_offset = local_inode_index * inode_size + blockgroup_descriptor->address_of_inode_table * block_size;

    fs_inode *inode = fs_xmalloc(inode_size);
    int ret;

    if (pread(fd, inode, inode_size, inode_block_offset) != inode_size) {
        ret = -EPROTO;
        goto cleanup_inode;
    }
    // printf("inode file size: %ud\n", inode->size_lower);

    if (inode->hard_link_count == 0) {
        ret = -ENOENT;
        goto cleanup_inode;
    }

    *inode_ptr = inode;
    return 0;

cleanup_inode:
    free(inode);
    return ret;
}

int get_inode_block_address_by_index(int fd, fs_inode *inode, uint32_t block_size, uint32_t inode_index,
                                     uint32_t *block_address_ptr, int64_t *single_indirect_block_cache_id,
                                     int64_t *double_indirect_block_cache_id, int64_t *triple_indirect_block_cache_id, char* cache) {
    uint32_t used_blocks = (inode->size_lower + block_size - 1) / block_size;
    if (inode_index >= used_blocks) return -ERANGE;

    uint32_t block_address;

    if (inode_index < DIRECT_POINTERS) { // Direct block
        block_address = inode->direct_block_pointers[inode_index];
        goto end_get_id;
    }

    char *single_cache = cache;                  // Block 1
    char *double_cache = cache + block_size;     // Block 2
    char *triple_cache = cache + 2 * block_size; // Block 3

    if (inode_index < (DIRECT_POINTERS + block_size / BLOCK_ADDRESS_SIZE)) { // Single indirect block
        uint32_t singly_offset = inode_index - DIRECT_POINTERS;

        if (*single_indirect_block_cache_id != (int64_t) inode->singly_indirect_block) {
            if (pread(fd, single_cache, block_size, inode->singly_indirect_block * block_size) != block_size) {
                return -EIO;
            }
            *single_indirect_block_cache_id = (int64_t) inode->singly_indirect_block;
        }

        block_address = ((uint32_t*)single_cache)[singly_offset];
        goto end_get_id;
    }

    if (inode_index < (DIRECT_POINTERS + block_size / BLOCK_ADDRESS_SIZE +
                       (block_size / BLOCK_ADDRESS_SIZE) * (block_size / BLOCK_ADDRESS_SIZE))) { // Double indirect block
        uint32_t doubly_offset = inode_index - (DIRECT_POINTERS + block_size / BLOCK_ADDRESS_SIZE);
        uint32_t singly_index = doubly_offset / (block_size / BLOCK_ADDRESS_SIZE);
        uint32_t indirect_index = doubly_offset % (block_size / BLOCK_ADDRESS_SIZE);

        if (*double_indirect_block_cache_id != (int64_t) inode->doubly_indirect_block) {
            if (pread(fd, double_cache, block_size, inode->doubly_indirect_block * block_size) != block_size) {
                return -EIO;
            }
            *double_indirect_block_cache_id = (int64_t) inode->doubly_indirect_block;
        }

        uint32_t singly_indirect_block_address = ((uint32_t*)double_cache)[singly_index];

        if (*single_indirect_block_cache_id != (int64_t) singly_indirect_block_address) {
            if (pread(fd, single_cache, block_size, singly_indirect_block_address * block_size) != block_size) {
                return -EIO;
            }
            *single_indirect_block_cache_id = (int64_t) singly_indirect_block_address;
        }

        block_address = ((uint32_t*)single_cache)[indirect_index];
        goto end_get_id;
    }

    { // Triple indirect block
        uint32_t triply_offset = inode_index - (DIRECT_POINTERS + block_size / BLOCK_ADDRESS_SIZE +
                                                (block_size / BLOCK_ADDRESS_SIZE) * (block_size / BLOCK_ADDRESS_SIZE));
        uint32_t doubly_index = triply_offset / ((block_size / BLOCK_ADDRESS_SIZE) * (block_size / BLOCK_ADDRESS_SIZE));
        uint32_t singly_index = (triply_offset / (block_size / BLOCK_ADDRESS_SIZE)) % (block_size / BLOCK_ADDRESS_SIZE);
        uint32_t indirect_index = triply_offset % (block_size / BLOCK_ADDRESS_SIZE);

        if (*triple_indirect_block_cache_id != (int64_t) inode->triply_indirect_block) {
            if (pread(fd, triple_cache, block_size, inode->triply_indirect_block * block_size) != block_size) {
                return -EIO;
            }
            *triple_indirect_block_cache_id = (int64_t) inode->triply_indirect_block;
        }

        uint32_t double_indirect_block_address = ((uint32_t*)triple_cache)[doubly_index];

        if (*double_indirect_block_cache_id != (int64_t) double_indirect_block_address) {
            if (pread(fd, double_cache, block_size, double_indirect_block_address * block_size) != block_size) {
                return -EIO;
            }
            *double_indirect_block_cache_id = (int64_t) double_indirect_block_address;
        }

        uint32_t singly_indirect_block_address = ((uint32_t*)double_cache)[singly_index];

        if (*single_indirect_block_cache_id != (int64_t) singly_indirect_block_address) {
            if (pread(fd, single_cache, block_size, singly_indirect_block_address * block_size) != block_size) {
                return -EIO;
            }
            *single_indirect_block_cache_id = (int64_t) singly_indirect_block_address;
        }

        block_address = ((uint32_t*)single_cache)[indirect_index];
    }

end_get_id:
    *block_address_ptr = block_address;
    if (inode_index == used_blocks - 1) return 0;
    return 1;
}

int read_block(int fd, uint32_t block_size, uint32_t block_address, char *buffer) {
    if (pread(fd, buffer, block_size, block_address * block_size) != block_size) return -EIO;
    return 0;
}

void print_extra_data(char *extra_data, int extra_data_size) {
    printf("Extra Data:\n");
    for (int i = 0; i < extra_data_size; i++) {
        if (extra_data[i] == 0)
            printf("  0 ");
        else
            printf("%03d ", extra_data[i]);

        if ((i + 1) % 8 == 0)
            printf("\t");

        if ((i + 1) % 32 == 0)
            printf("\n");

        if ((i + 1) % 1024 == 0)
            printf("\n\n");
    }
}

void print_inode_info(fs_inode *inode, uint16_t inode_size, int inode_number, uint32_t block_size, int fd) {
    uint16_t type = inode->type_and_permissions & 0xF000;
    if (type == 0x4000) {
        printf("Inode %d is a directory.\n", inode_number);
    } else if (type == 0x8000) {
        printf("Inode %d is a file.\n", inode_number);

        // Read and print the content from the first block if it’s a file
        printf("File content from the first block:\n");
        char *buffer = fs_xmalloc(3 * block_size);
        char *buffer_page = fs_xmalloc(block_size);
        int64_t single = -1;
        int64_t double_ = -1;
        int64_t triple = -1;
        uint32_t block_addr = 0;
        get_inode_block_address_by_index(fd, inode, (uint32_t) block_size, 0, &block_addr, &single, &double_, &triple, buffer);
        read_block(fd, block_size, block_addr, buffer_page);
        print_extra_data(buffer_page, (int) block_size);
        free(buffer);
        free(buffer_page);
    } else {
        printf("Inode %d is neither a file nor a directory.\n", inode_number);
    }

    printf("Inode Information:\n");
    printf("Type and Permissions: 0x%04X\n", inode->type_and_permissions);
    printf("User ID: %u\n", inode->user_id);
    printf("Lower 32 bits of size: %u bytes\n", inode->size_lower);
    printf("Last Access Time: %u\n", inode->last_access_time);
    printf("Creation Time: %u\n", inode->creation_time);
    printf("Last Modification Time: %u\n", inode->last_modification_time);
    printf("Deletion Time: %u\n", inode->deletion_time);
    printf("Group ID: %u\n", inode->group_id);
    printf("Hard Link Count: %u\n", inode->hard_link_count);
    printf("Disk Sector Count: %u\n", inode->disk_sector_count);
    printf("Flags: 0x%08X\n", inode->flags);
    printf("OS-Specific Value #1: %u\n", inode->os_specific_value_1);

    for (int i = 0; i < 12; i++) {
        printf("Direct Block Pointer %d: %u\n", i, inode->direct_block_pointers[i]);
    }

    printf("Singly Indirect Block Pointer: %u\n", inode->singly_indirect_block);
    printf("Doubly Indirect Block Pointer: %u\n", inode->doubly_indirect_block);
    printf("Triply Indirect Block Pointer: %u\n", inode->triply_indirect_block);
    printf("Generation Number: %u\n", inode->generation_number);
    printf("Extended Attribute Block: %u\n", inode->extended_attribute_block);
    printf("Upper 32 bits of size or Directory ACL: %u\n", inode->size_upper_or_directory_acl);
    printf("Fragment Block Address: %u\n", inode->fragment_block_address);

    for (int i = 0; i < 12; i++) {
        printf("OS-Specific Value #2 Byte %d: %u\n", i, inode->os_specific_value_2[i]);
    }

    print_extra_data(inode->extra_data, inode_size - 128);
}
