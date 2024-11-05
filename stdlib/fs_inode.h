//
// Created by andrei on 10/30/24.
//


#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

#include "fs_block_group_descriptor_table.h"
#include "fs_superblock.h"

#ifndef FS_INODE_H
#define FS_INODE_H

#define DIRECT_POINTERS 12
#define BLOCK_ADDRESS_SIZE 4

#pragma pack(push, 1)

typedef struct {
    uint16_t type_and_permissions; // Byte 0-1: Type and Permissions
    uint16_t user_id; // Byte 2-3: User ID
    uint32_t size_lower; // Byte 4-7: Lower 32 bits of size in bytes
    uint32_t last_access_time; // Byte 8-11: Last Access Time (POSIX)
    uint32_t creation_time; // Byte 12-15: Creation Time (POSIX)
    uint32_t last_modification_time; // Byte 16-19: Last Modification Time (POSIX)
    uint32_t deletion_time; // Byte 20-23: Deletion Time (POSIX)
    uint16_t group_id; // Byte 24-25: Group ID
    uint16_t hard_link_count; // Byte 26-27: Hard link count
    uint32_t disk_sector_count; // Byte 28-31: Disk sectors in use
    uint32_t flags; // Byte 32-35: Flags
    uint32_t os_specific_value_1; // Byte 36-39: OS-specific value #1
    uint32_t direct_block_pointers[DIRECT_POINTERS]; // Byte 40-87: Direct Block Pointers 0-11
    uint32_t singly_indirect_block; // Byte 88-91: Singly Indirect Block Pointer
    uint32_t doubly_indirect_block; // Byte 92-95: Doubly Indirect Block Pointer
    uint32_t triply_indirect_block; // Byte 96-99: Triply Indirect Block Pointer
    uint32_t generation_number; // Byte 100-103: Generation number
    uint32_t extended_attribute_block; // Byte 104-107: Extended attribute block / reserved
    uint32_t size_upper_or_directory_acl; // Byte 108-111: Upper 32 bits of size (files) or Directory ACL (directories)
    uint32_t fragment_block_address; // Byte 112-115: Fragment block address
    uint8_t os_specific_value_2[12]; // Byte 116-127: OS-specific value #2
    char extra_data[]; // Flexible array member for additional inode data
} fs_inode;

#pragma pack(pop)

int init_inode(int fd, fs_superblock *superblock, fs_blockgroup_descriptor *blockgroup_descriptor, int inode_number,
               fs_inode **inode_ptr);

int get_inode_block_address_by_index(int fd, fs_inode *inode, uint32_t block_size, uint32_t inode_index,
                                     uint32_t *block_address_ptr, int64_t *single_indirect_cache_begin_id, int64_t *double_indirect_cache_begin_id, int64_t
                                     *triple_indirect_cache_begin_id, char* cache);

int read_block(int fd, uint32_t block_size, uint32_t block_address, char *buffer);

void print_extra_data(char *extra_data, int extra_data_size) ;

void print_inode_info(fs_inode *inode, uint16_t inode_size, int inode_number, uint32_t block_size, int fd);

#endif // FS_INODE_H
