#pragma once
#include "fs_inode.h"
#include <stdint.h>

#include "fs_dir_entry.h"
#include "fs_void_vector.h"

static const int ROOT_INODE_ID = 2;

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

typedef size_t (*on_write_t)(const void* buffer, size_t bytes_to_write, off_t offset, void* context);

static inline size_t file_write_callback(const void* buffer, size_t bytes_to_write, off_t bytes_read, void* context) {
    const int out = *(int*)context;
    return pwrite(out, buffer, bytes_to_write, bytes_read);
}

typedef int (*on_file_dump_t)(const int img, int inode, void* context);

int dump_ext2_file(int img, int inode_nr, void* context, on_write_t on_write);

int dump_ext2_file_part(int img, int inode_nr, void* context, on_write_t on_write, uint32_t offset, uint32_t size);

static inline size_t file_dump_callback(const int img, int inode, void* context) {
    return dump_ext2_file(img, inode, context, file_write_callback);
}

void parse_path_to_segments(const char *path, fs_vector *segments);

int dump_ext2_file_on_path(int img, const char *path, void* context, on_file_dump_t on_dump);


struct ext2_entity {
    int img;
    const char *path;
    uint32_t inode;
    struct ext2_fs *file_system;
};

int ext2_entity_init(int img, const char *path, struct ext2_entity** entity);

void ext2_entity_free(struct ext2_entity *entity);