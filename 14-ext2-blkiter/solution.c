#include "solution.h"
#include "fs_malloc.h"
#include "fs_inode.h"
#include "fs_superblock.h"
#include "fs_block_group_descriptor_table.h"
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>


int init_superblock(int fd, fs_superblock *superblock) {
    if (pread(fd, superblock, sizeof(fs_superblock), 1024) != sizeof(fs_superblock)) return -EPROTO;
    if (superblock->s_magic != 0xEF53) return -EPROTO;
    return 0;
}

int init_blockgroup_descriptor(int fd, fs_superblock *superblock, fs_blockgroup_descriptor *blockgroup_descriptor) {
    __off64_t begining_of_blockgroup_descriptor;
    if (superblock->s_log_block_size_kbytes == 0) begining_of_blockgroup_descriptor = (1 << 11);
    else begining_of_blockgroup_descriptor = 1 << (10 + superblock->s_log_block_size_kbytes);
    if (pread(fd, blockgroup_descriptor, sizeof(fs_blockgroup_descriptor), begining_of_blockgroup_descriptor)
        != sizeof(fs_blockgroup_descriptor)) {
        return -EPROTO;
    }

    return 0;
}


struct ext2_fs {
    fs_superblock *superblock;
    fs_blockgroup_descriptor *blockgroup_descriptor;
    int fd;
};


struct ext2_blkiter {
    fs_inode *inode;
    u_int32_t iterator_block_index;
    int fd;
    u_int32_t block_size;
};

int ext2_fs_init(struct ext2_fs **fs, int fd) {
    int ret;
    fs_superblock *superblock = NULL;
    fs_blockgroup_descriptor *blockgroup_descriptor = NULL;
    struct ext2_fs *file_system = NULL;

    superblock = fs_xmalloc(sizeof(fs_superblock));
    ret = init_superblock(fd, superblock);
    if (ret != 0) goto clenup;

    blockgroup_descriptor = fs_xmalloc(sizeof(fs_blockgroup_descriptor));
    ret = init_blockgroup_descriptor(fd, superblock, blockgroup_descriptor);
    if (ret != 0) goto clenup;
    // print_superblock_info(superblock);
    // print_block_group_descriptor_info(blockgroup_descriptor);

    file_system = fs_xmalloc(sizeof(struct ext2_fs));
    file_system->fd = fd;
    file_system->blockgroup_descriptor = blockgroup_descriptor;
    file_system->superblock = superblock;
    *fs = file_system;
    return 0;

clenup:
    free(superblock);
    free(blockgroup_descriptor);
    free(file_system);
    return ret;
}

void ext2_fs_free(struct ext2_fs *fs) {
    if (fs == NULL) return;
    fs_superblock *superblock = fs->superblock;
    fs_blockgroup_descriptor *blockgroup_descriptor = fs->blockgroup_descriptor;
    free(superblock);
    free(blockgroup_descriptor);
    close(fs->fd);
    free(fs);
}

int ext2_blkiter_init(struct ext2_blkiter **i, struct ext2_fs *fs, int ino) {
    struct ext2_blkiter *blkiter = fs_xmalloc(sizeof(struct ext2_blkiter));
    blkiter->iterator_block_index = 0;
    blkiter->block_size = 1 << (10 + fs->superblock->s_log_block_size_kbytes);

    int ret = init_inode(fs->fd, fs->superblock, fs->blockgroup_descriptor, ino, &blkiter->inode);
    // print_inode_info(blkiter->inode, fs->superblock->s_inode_size, ino, blkiter->block_size, fs->fd);
    if (ret != 0) goto cleenup;

    blkiter->fd = fs->fd;
    *i = blkiter;
    return 0;

cleenup:
    free(blkiter);
    return ret;
}

int ext2_blkiter_next(struct ext2_blkiter *i, int *blkno) {
    uint32_t block_number = 0;

    int ret = get_inode_block_address_by_index(i->fd, i->inode, i->block_size, i->iterator_block_index++,
                                               &block_number);
    // printf("%i\n", block_number);
    if (ret == -ERANGE) {
        // printf("Stop!");
        return 0;
    }
    if (ret != 0) {
        // printf("Stop!");
        return ret;
    };
    *blkno = (int) block_number;
    return 1;
}

void ext2_blkiter_free(struct ext2_blkiter *i) {
    if (i == NULL) return;
    free(i->inode);
    free(i);
}
