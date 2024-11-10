#include "fs_malloc.h"
#include "fs_ext2.h"
#include "fs_inode.h"
#include "fs_superblock.h"
#include "fs_block_group_descriptor_table.h"
#include <stdint.h>
#include <unistd.h>

int ext2_fs_init(struct ext2_fs **fs, int fd) {
    int ret;
    fs_superblock *superblock = NULL;
    struct ext2_fs *file_system = NULL;

    superblock = fs_xmalloc(sizeof(fs_superblock));
    ret = init_superblock(fd, superblock);
    if (ret != 0) goto clenup;

    file_system = fs_xmalloc(sizeof(struct ext2_fs));
    file_system->fd = fd;
    file_system->superblock = superblock;
    *fs = file_system;
    return 0;

clenup:
    free(superblock);
    free(file_system);
    return ret;
}

void ext2_fs_free(struct ext2_fs *fs) {
    if (fs == NULL) return;
    fs_superblock *superblock = fs->superblock;
    free(superblock);
    close(fs->fd);
    free(fs);
}

int ext2_blkiter_init(struct ext2_blkiter **i, struct ext2_fs *fs, int ino) {
    fs_blockgroup_descriptor *blockgroup_descriptor = fs_xmalloc(sizeof(fs_blockgroup_descriptor));;
    u_int32_t block_size = 1 << (10 + fs->superblock->s_log_block_size_kbytes);
    struct ext2_blkiter *blkiter = fs_xmalloc(sizeof(struct ext2_blkiter) + 3 * block_size);
    blkiter->iterator_block_index = 0;
    blkiter->block_size = block_size;
    blkiter->single_indirect_block_cache_id = -1;
    blkiter->double_indirect_block_cache_id = -1;
    blkiter->triple_indirect_block_cache_id = -1;
    blkiter->blockgroup_descriptor = blockgroup_descriptor;


    uint32_t block_group = (ino - 1) / fs->superblock->s_inodes_per_group;

    int ret = init_blockgroup_descriptor(fs->fd, fs->superblock, blockgroup_descriptor, block_group);
    if (ret != 0) goto cleenup;

    ret = init_inode(fs->fd, fs->superblock, blockgroup_descriptor, ino, &blkiter->inode);
    // print_inode_info(blkiter->inode, fs->superblock->s_inode_size, ino, blkiter->block_size, fs->fd);
    if (ret != 0) goto cleenup;

    blkiter->fd = fs->fd;
    *i = blkiter;
    return 0;

cleenup:
    free(blockgroup_descriptor);
    free(blkiter);
    return ret;
}

int ext2_blkiter_next(struct ext2_blkiter *i, int *blkno) {
    uint32_t block_number = 0;

    int ret = get_inode_block_address_by_index(i->fd, i->iterator_block_index++, &block_number, i);
    if (ret == -ERANGE) return 0;
    if (ret < 0) { return ret; }
    *blkno = (int) block_number;
    return 1;
}

void ext2_blkiter_free(struct ext2_blkiter *i) {
    if (i == NULL) return;
    free(i->inode);
    free(i->blockgroup_descriptor);
    free(i);
}

int get_inode_block_address_by_index(int fd, uint32_t inode_index,
                                     uint32_t *block_address_ptr, struct ext2_blkiter *bllkiter) {
    uint32_t block_size = bllkiter->block_size;
    uint32_t used_blocks = (bllkiter->inode->size_lower + block_size - 1) / block_size;
    if (inode_index >= used_blocks) return -ERANGE;

    uint32_t block_address;

    if (inode_index < DIRECT_POINTERS) {
        // Direct block
        block_address = bllkiter->inode->direct_block_pointers[inode_index];
        goto end_get_id;
    }

    char *cache = bllkiter->indirect_pointer_cache;
    char *single_cache = cache; // Block 1
    char *double_cache = cache + block_size; // Block 2
    char *triple_cache = cache + 2 * block_size; // Block 3

    if (inode_index < (DIRECT_POINTERS + block_size / BLOCK_ADDRESS_SIZE)) {
        // Single indirect block
        uint32_t singly_offset = inode_index - DIRECT_POINTERS;

        if (bllkiter->single_indirect_block_cache_id != (int64_t) bllkiter->inode->singly_indirect_block) {
            if (pread(fd, single_cache, block_size, bllkiter->inode->singly_indirect_block * block_size) !=
                block_size) {
                return -EIO;
            }
            bllkiter->single_indirect_block_cache_id = (int64_t) bllkiter->inode->singly_indirect_block;
        }

        block_address = ((uint32_t *) single_cache)[singly_offset];
        goto end_get_id;
    }

    if (inode_index < DIRECT_POINTERS + block_size / BLOCK_ADDRESS_SIZE + block_size / BLOCK_ADDRESS_SIZE * (block_size / BLOCK_ADDRESS_SIZE)) {
        // Double indirect block
        uint32_t doubly_offset = inode_index - (DIRECT_POINTERS + block_size / BLOCK_ADDRESS_SIZE);
        uint32_t singly_index = doubly_offset / (block_size / BLOCK_ADDRESS_SIZE);
        uint32_t indirect_index = doubly_offset % (block_size / BLOCK_ADDRESS_SIZE);

        if (bllkiter->double_indirect_block_cache_id != (int64_t) bllkiter->inode->doubly_indirect_block) {
            if (pread(fd, double_cache, block_size, bllkiter->inode->doubly_indirect_block * block_size) !=
                block_size) {
                return -EIO;
            }
            bllkiter->double_indirect_block_cache_id = (int64_t) bllkiter->inode->doubly_indirect_block;
        }

        uint32_t singly_indirect_block_address = ((uint32_t *) double_cache)[singly_index];

        if (bllkiter->single_indirect_block_cache_id != (int64_t) singly_indirect_block_address) {
            if (pread(fd, single_cache, block_size, singly_indirect_block_address * block_size) != block_size) {
                return -EIO;
            }
            bllkiter->single_indirect_block_cache_id = (int64_t) singly_indirect_block_address;
        }

        block_address = ((uint32_t *) single_cache)[indirect_index];
        goto end_get_id;
    } {
        // Triple indirect block
        uint32_t triply_offset = inode_index - (DIRECT_POINTERS + block_size / BLOCK_ADDRESS_SIZE +
                                                (block_size / BLOCK_ADDRESS_SIZE) * (block_size / BLOCK_ADDRESS_SIZE));
        uint32_t doubly_index = triply_offset / ((block_size / BLOCK_ADDRESS_SIZE) * (block_size / BLOCK_ADDRESS_SIZE));
        uint32_t singly_index = (triply_offset / (block_size / BLOCK_ADDRESS_SIZE)) % (block_size / BLOCK_ADDRESS_SIZE);
        uint32_t indirect_index = triply_offset % (block_size / BLOCK_ADDRESS_SIZE);

        if (bllkiter->triple_indirect_block_cache_id != (int64_t) bllkiter->inode->triply_indirect_block) {
            if (pread(fd, triple_cache, block_size, bllkiter->inode->triply_indirect_block * block_size) !=
                block_size) {
                return -EIO;
            }
            bllkiter->triple_indirect_block_cache_id = (int64_t) bllkiter->inode->triply_indirect_block;
        }

        uint32_t double_indirect_block_address = ((uint32_t *) triple_cache)[doubly_index];

        if (bllkiter->double_indirect_block_cache_id != (int64_t) double_indirect_block_address) {
            if (pread(fd, double_cache, block_size, double_indirect_block_address * block_size) != block_size) {
                return -EIO;
            }
            bllkiter->double_indirect_block_cache_id = (int64_t) double_indirect_block_address;
        }

        uint32_t singly_indirect_block_address = ((uint32_t *) double_cache)[singly_index];

        if (bllkiter->single_indirect_block_cache_id != (int64_t) singly_indirect_block_address) {
            if (pread(fd, single_cache, block_size, singly_indirect_block_address * block_size) != block_size) {
                return -EIO;
            }
            bllkiter->single_indirect_block_cache_id = (int64_t) singly_indirect_block_address;
        }

        block_address = ((uint32_t *) single_cache)[indirect_index];
    }

end_get_id:
    *block_address_ptr = block_address;
    if (inode_index == used_blocks - 1) return 0;
    return 1;
}
