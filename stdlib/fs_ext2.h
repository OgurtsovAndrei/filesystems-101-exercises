#pragma once
#include "fs_inode.h"
#include <stdint.h>

struct ext2_fs {
    fs_superblock *superblock;
    int fd;
};

struct ext2_blkiter {
    int fd;
    uint32_t iterator_block_index;
    uint32_t block_size;
    fs_inode *inode;
    fs_blockgroup_descriptor *blockgroup_descriptor;
    int64_t single_indirect_block_cache_id;
    int64_t double_indirect_block_cache_id;
    int64_t triple_indirect_block_cache_id;
    char indirect_pointer_cache[];
};

/**
   Allocate and initialise the reader of an ext2 file system. An image
   to read is open at file descriptor @fd. @fs takes the ownership of @fd.

   Return values:
   * 0 if successful,
   * a (negative) errno code if an IO or memory allocation error occurred,
   * -EPROTO if the superblock or blockgroup descriptors are corrupted.
 */
int ext2_fs_init(struct ext2_fs **fs, int fd);

/**
   Free resources associated with an ext2 reader @fs.

   Note: ext2_fs_free(NULL) is a no-op.
 */
void ext2_fs_free(struct ext2_fs *fs);

/**
   Allocate and initialise an iterator over blocks of an inode @ino.
   The inode data is not sparse. The iterator must return block numbers
   in the same order as they would be fetched by a sequential read.

   Hint: you may compare your iterator with results produced by `blocks`
   command of `debugfs`.

   Return values:
   * 0 if successful,
   * a (negative) errno code if an IO or memory allocation error occurred,
   * -EINVAL if the inode number is out range,
   * -ENOENT if the inode number is valid, but the inode is not in use,
   * -EPROTO if the inode has a corrupted block list (e.g. block numbers
             are out of range).
 */
int ext2_blkiter_init(struct ext2_blkiter **i, struct ext2_fs *fs, int ino);

/**
   Advance the iterator, and return the next block number in @blkno.

   Return values:
   * +1 if successful and @blkno was updated with a new block number,
   * 0 if successful and the iteration is over,
   * a (negative) errno code if an error occurred.
 */
int ext2_blkiter_next(struct ext2_blkiter *i, int *blkno);

/**
   Free resources associated with an iterator @i.

   Note: ext2_blkiter_free(NULL) is a no-op.
 */
void ext2_blkiter_free(struct ext2_blkiter *i);

int get_inode_block_address_by_index(
    int fd,
    uint32_t inode_index,
    uint32_t *block_address_ptr,
    struct ext2_blkiter *bllkiter
);

/**
   Implement this function to copy the content of an inode @inode_nr
   to a file descriptor @out. @img is a file descriptor of an open
   ext2 image.

   It suffices to support single- and double-indirect blocks.

   If a copy was successful, return 0. If an error occurred during
   a read or a write, return -errno.
*/
int dump_ext2_file(int img, int inode_nr, int out);
