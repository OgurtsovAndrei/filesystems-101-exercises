//
// Created by andrei on 10/30/24.
//

#ifndef FS_SUPERBLOCK_H
#define FS_SUPERBLOCK_H

#include <stdint.h>

#pragma pack(push, 1)

typedef struct {
    // Base Superblock Fields 84 bytes [0-83]
    uint32_t s_inodes_count;             // Total number of inodes
    uint32_t s_blocks_count;             // Total number of blocks
    uint32_t s_us_reserved_blocks_count; // Number of reserved blocks for superuser
    uint32_t s_free_blocks_count;        // Number of unallocated blocks
    uint32_t s_free_inodes_count;        // Number of unallocated inodes
    uint32_t s_first_data_block;         // Block number of the first data block
    uint32_t s_log_block_size_kbytes;    // Block size (logarithmic)
    uint32_t s_log_frag_size_kbytes;     // Fragment size (logarithmic)
    // 32 bytes
    uint32_t s_blocks_per_group;         // Number of blocks per group
    uint32_t s_frags_per_group;          // Number of fragments per group
    uint32_t s_inodes_per_group;         // Number of inodes per group
    uint32_t s_mtime;                    // Last mount time
    uint32_t s_wtime;                    // Last write time
    uint16_t s_mnt_count;                // Number of mounts since last check
    uint16_t s_max_mnt_count;            // Max number of mounts allowed  before check
    uint16_t s_magic;                    // Magic number (should be 0xEF53) (Ext2 signature)
    uint16_t s_state;                    // File system state
    uint16_t s_errors;                   // Behaviour when detecting errors
    uint16_t s_minor_rev_level;          // Minor revision level
    // 64 bytes
    uint32_t s_lastcheck;                // Time of last check
    uint32_t s_checkinterval;            // Interval between forced consistency checks
    uint32_t s_creator_os;               // OS where the filesystem was created
    uint32_t s_rev_level;                // Revision level (Major portion of version)
    uint16_t s_def_resuid;               // Default user ID that can use reserved blocks
    uint16_t s_def_resgid;               // Default group ID that can use reserved blocks
    // Extended Superblock Fields [84 - 235]
    uint32_t s_first_ino;                // First non-reserved inode
    uint16_t s_inode_size;               // Size of inode structure
    uint16_t s_block_group_nr;           // Block group number of this superblock
    uint32_t s_feature_compat;           // Compatible feature set
    uint32_t s_feature_incompat;         // Incompatible feature set
    uint32_t s_feature_ro_compat;        // Read-only compatible feature set
    uint8_t s_uuid[16];                  // 128-bit (16 bytes) filesystem ID
    char s_volume_name[16];              // Volume name
    char s_last_mounted[64];             // Path where the filesystem was last mounted
    // 200 bytes
    uint32_t s_algorithm_usage_bitmap;   // For compression
    uint8_t s_prealloc_blocks;           // Number of blocks to preallocate
    uint8_t s_prealloc_dir_blocks;       // Number of blocks to preallocate for directories
    uint16_t s_padding1;                 // Alignment padding
    uint8_t s_journal_uuid[16];          // UUID of journal superblock
    uint32_t s_journal_inum;             // Inode number of journal file
    uint32_t s_journal_dev;              // Device number of journal file
    uint32_t s_last_orphan;              // Start of list of orphaned inodes to delete (Head of orphan inode list)
    // 236 bytes
    uint8_t s_reserved[788];            // Padding to make the struct 1024 bytes
} fs_superblock;

#pragma pack(pop)

int init_superblock(int fd, fs_superblock *superblock);

void print_superblock_info(const fs_superblock *superblock);

#endif // FS_SUPERBLOCK_H
