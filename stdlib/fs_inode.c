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

    uint16_t inode_size = superblock->s_inode_size;
    __off64_t block_size = 1 << (10 + superblock->s_log_block_size_kbytes);
    __off64_t inode_block_offset = (inode_number - 1) * inode_size + blockgroup_descriptor->address_of_inode_table *
                                   block_size;

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

int get_inode_block_address_by_index(int fd, fs_inode *inode, uint32_t block_size, uint32_t block_index,
                                     uint32_t *block_address_ptr) {
    uint32_t used_blocks = (inode->size_lower + block_size - 1) / block_size;
    if (block_index >= used_blocks) return -ERANGE;

    uint32_t block_address;

    if (block_index < DIRECT_POINTERS) {
        block_address = inode->direct_block_pointers[block_index];
    } else if (block_index < (DIRECT_POINTERS + block_size / BLOCK_ADDRESS_SIZE)) {
        uint32_t singly_offset = block_index - DIRECT_POINTERS;
        uint32_t indirect_block_offset = inode->singly_indirect_block * block_size + singly_offset * BLOCK_ADDRESS_SIZE;

        if (pread(fd, &block_address, BLOCK_ADDRESS_SIZE, indirect_block_offset) != BLOCK_ADDRESS_SIZE) {
            return -EIO;
        }
    } else if (block_index < (DIRECT_POINTERS + block_size / BLOCK_ADDRESS_SIZE + (block_size / BLOCK_ADDRESS_SIZE) *
                              (block_size / BLOCK_ADDRESS_SIZE))) {
        uint32_t doubly_offset = block_index - (DIRECT_POINTERS + block_size / BLOCK_ADDRESS_SIZE);
        uint32_t singly_index = doubly_offset / (block_size / BLOCK_ADDRESS_SIZE);
        uint32_t indirect_index = doubly_offset % (block_size / BLOCK_ADDRESS_SIZE);

        uint32_t indirect_block_offset = inode->doubly_indirect_block * block_size + singly_index * BLOCK_ADDRESS_SIZE;

        if (pread(fd, &block_address, BLOCK_ADDRESS_SIZE, indirect_block_offset) != BLOCK_ADDRESS_SIZE) {
            return -EIO;
        }

        if (pread(fd, &block_address, BLOCK_ADDRESS_SIZE,
                  block_address * block_size + indirect_index * BLOCK_ADDRESS_SIZE) != BLOCK_ADDRESS_SIZE) {
            return -EIO;
        }
    } else {
        uint32_t triply_offset = block_index - (DIRECT_POINTERS + block_size / BLOCK_ADDRESS_SIZE +
                                                (block_size / BLOCK_ADDRESS_SIZE) * (block_size / BLOCK_ADDRESS_SIZE));
        uint32_t doubly_index = triply_offset / ((block_size / BLOCK_ADDRESS_SIZE) * (block_size / BLOCK_ADDRESS_SIZE));
        uint32_t singly_index = (triply_offset / (block_size / BLOCK_ADDRESS_SIZE)) % (block_size / BLOCK_ADDRESS_SIZE);
        uint32_t indirect_index = triply_offset % (block_size / BLOCK_ADDRESS_SIZE);

        uint32_t indirect_block_offset = inode->triply_indirect_block * block_size + doubly_index * BLOCK_ADDRESS_SIZE;

        if (pread(fd, &block_address, BLOCK_ADDRESS_SIZE, indirect_block_offset) != BLOCK_ADDRESS_SIZE) {
            return -EIO;
        }

        // Read address from doubly indirect block
        if (pread(fd, &block_address, BLOCK_ADDRESS_SIZE,
                  block_address * block_size + singly_index * BLOCK_ADDRESS_SIZE) != BLOCK_ADDRESS_SIZE) {
            return -EIO;
        }

        if (pread(fd, &block_address, BLOCK_ADDRESS_SIZE,
                  block_address * block_size + indirect_index * BLOCK_ADDRESS_SIZE) != BLOCK_ADDRESS_SIZE) {
            return -EIO;
        }
    }

    *block_address_ptr = block_address;
    if (block_index == used_blocks - 1) return 0;
    return 1;
}

int read_inode_block(int fd, fs_inode *inode, uint32_t block_size, uint32_t block_index, char *buffer) {
    uint32_t block_address;
    int ret = get_inode_block_address_by_index(fd, inode, block_size, block_index, &block_address);
    if (ret != 0) return ret;
    if (pread(fd, buffer, block_size, block_address * block_size) != block_size) return -EIO;
    return 0;
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
        char *buffer = fs_xmalloc(block_size);
        read_inode_block(fd, inode, (uint32_t) block_size, 0, buffer);
        print_extra_data(buffer, block_size);
        free(buffer);
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
