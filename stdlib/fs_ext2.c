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

    int ret = get_inode_block_address_by_index(i->fd, i->inode, i->block_size, i->iterator_block_index++, &block_number,
                                               &i->single_indirect_block_cache_id,
                                               &i->double_indirect_block_cache_id,
                                               &i->triple_indirect_block_cache_id,
                                               i->indirect_pointer_cache);
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
