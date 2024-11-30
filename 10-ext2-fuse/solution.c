#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 31
#endif

#include "solution.h"

#include <err.h>
#include <fuse.h>
#include <errno.h>
#include <fs_malloc.h>
#include <string.h>
#include <stdlib.h>

#include "fs_dir_entry.h"
#include "fs_ext2.h"

struct ext2_context {
    struct ext2_fs *fs;
};

static struct ext2_context ctx;

static int ext2_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    // printf("ext2 getattr - path: %s\n", path);

    (void) fi;
    int ret = 0;
    struct ext2_entity *entity = NULL;
    fs_inode *inode = NULL;

    memset(stbuf, 0, sizeof(struct stat));

    ret = ext2_entity_init(ctx.fs->fd, path, &entity);
    if (ret != 0) {
        // printf("Error finding entity for path: %s\n", path);
        ret = -ENOENT;
        goto only_entry_cleanup;
    }

    fs_blockgroup_descriptor *blockgroup_descriptor = fs_xmalloc(sizeof(fs_blockgroup_descriptor));

    uint32_t block_group = (entity->inode - 1) / ctx.fs->superblock->s_inodes_per_group;
    ret = init_blockgroup_descriptor(ctx.fs->fd, ctx.fs->superblock, blockgroup_descriptor, block_group);
    if (ret != 0) goto cleanup;


    ret = init_inode(ctx.fs->fd, ctx.fs->superblock, blockgroup_descriptor, (int) entity->inode, &inode);
    if (ret != 0) { goto cleanup; }

    stbuf->st_mode = inode->type_and_permissions; // File mode
    stbuf->st_nlink = inode->hard_link_count; // Number of hard links
    stbuf->st_uid = inode->user_id; // User ID
    stbuf->st_gid = inode->group_id; // Group ID
    stbuf->st_size = inode->size_lower; // File size (lower 32 bits)
    stbuf->st_blocks = inode->disk_sector_count; // Number of 512-byte blocks
    stbuf->st_atime = inode->last_access_time; // Last access time
    stbuf->st_mtime = inode->last_modification_time; // Last modification time
    stbuf->st_ctime = inode->creation_time; // Creation time

    if (S_ISDIR(inode->type_and_permissions)) {
        stbuf->st_mode |= S_IFDIR;
    } else {
        stbuf->st_mode |= S_IFREG;
    }


cleanup:
    if (blockgroup_descriptor) free(blockgroup_descriptor);
    if (inode && ret != 0) free(inode);
only_entry_cleanup:
    if (entity) ext2_entity_free(entity);
    return ret;
}

static int ext2_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
                        struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    // printf("ext2_readdir\n");
    (void) offset; // Offset is not used
    (void) fi; // File info is not used
    (void) flags; // Flags are not used

    int ret = 0;
    struct ext2_fs *file_system = NULL;
    struct ext2_blkiter *blkiter = NULL;

    if ((ret = ext2_fs_init(&file_system, ctx.fs->fd))) {
        return ret;
    }

    fs_vector path_segments;
    vector_init(&path_segments);
    parse_path_to_segments(path, &path_segments);

    VectorIterator iterator;
    vector_iterator_init(&iterator, &path_segments);

    int current_inode = ROOT_INODE_ID;

    while (vector_iterator_has_next(&iterator) && current_inode != 0) {
        fs_string *segment = vector_iterator_next(&iterator);

        if ((ret = ext2_blkiter_init(&blkiter, file_system, current_inode))) {
            ret = -ENOENT;
            goto cleanup;
        }

        fs_vector dir_entries;
        dump_directory(file_system, blkiter, &dir_entries);

        uint32_t segment_inode = 0;
        VectorIterator dir_entries_iterator;
        vector_iterator_init(&dir_entries_iterator, &dir_entries);

        while (vector_iterator_has_next(&dir_entries_iterator)) {
            dir_entry *entry = (dir_entry *) vector_iterator_next(&dir_entries_iterator);
            char *entry_name = get_name(entry);

            // Check if the current entry matches the segment
            if (strlen(entry_name) == segment->length &&
                memcmp(entry_name, segment->data, segment->length) == 0) {
                free(entry_name);

                // If it's the last segment, ensure it's a directory
                if (iterator.current == iterator.vector->size) {
                    if (!is_dir(entry)) {
                        ret = -ENOTDIR;
                        goto clean_segment;
                    }

                    segment_inode = entry->inode;
                    break;
                }

                segment_inode = entry->inode;
                break;
            }

            free(entry_name);
        }

        if (segment_inode == 0) {
            ret = -ENOENT;
        }

    clean_segment:
        current_inode = (int) segment_inode;
        vector_free_all_elements(&dir_entries, (void (*)(void *)) free_entry);
        vector_free(&dir_entries);
        ext2_blkiter_free(blkiter);
    }

    if ((ret = ext2_blkiter_init(&blkiter, file_system, current_inode))) {
        ret = -ENOENT;
        goto cleanup;
    }

    fs_vector dir_entries;
    dump_directory(file_system, blkiter, &dir_entries);

    VectorIterator dir_iterator;
    vector_iterator_init(&dir_iterator, &dir_entries);

    while (vector_iterator_has_next(&dir_iterator)) {
        dir_entry *entry = (dir_entry *) vector_iterator_next(&dir_iterator);
        char *entry_name = get_name(entry);
        filler(buf, entry_name, NULL, 0, 0); // Add entry to FUSE buffer
        free(entry_name);
    }

    vector_free_all_elements(&dir_entries, (void (*)(void *)) free_entry);
    vector_free(&dir_entries);

cleanup:
    vector_free_all_elements(&path_segments, (void (*)(void *)) fs_string_free);
    vector_free(&path_segments);
    ext2_blkiter_free(blkiter);
    ext2_fs_free(file_system);

    return ret;
}

static int ext2_open(const char *path, struct fuse_file_info *fi) {
    // printf("Ext2 open - path: %s\n", path);
    int ret;
    struct ext2_entity *entity = NULL;
    static fs_inode *inode_ptr = NULL;
    ret = ext2_entity_init(ctx.fs->fd, path, &entity);
    if (ret != 0) { goto cleanup; }

    fs_blockgroup_descriptor *blockgroup_descriptor = fs_xmalloc(sizeof(fs_blockgroup_descriptor));
    uint32_t block_group = (entity->inode - 1) / ctx.fs->superblock->s_inodes_per_group;

    ret = init_blockgroup_descriptor(ctx.fs->fd, ctx.fs->superblock, blockgroup_descriptor, block_group);
    if (ret != 0) goto cleanup;

    ret = init_inode(ctx.fs->fd, ctx.fs->superblock, blockgroup_descriptor, (int) entity->inode, &inode_ptr);
    if (ret != 0) goto cleanup;

    if ((fi->flags & O_ACCMODE) != O_RDONLY) { return -EROFS; }

cleanup:
    ext2_entity_free(entity);
    free(inode_ptr);
    free(blockgroup_descriptor);
    return ret;
}

struct read_context {
    char *buf; // Buffer to write data to
    size_t size; // Number of bytes requested by FUSE
    off_t offset; // Offset from where to start reading
    size_t written; // Number of bytes written to buffer
};

static size_t read_file_callback(const void *buffer, size_t bytes_to_write, off_t offset, void *context) {
    if (offset != 0) err(1, "read_file_callback");
    struct read_context *my_context = (struct read_context *) context;

    // Copy the data directly into the FUSE buffer
    memcpy(my_context->buf + my_context->written, buffer, bytes_to_write);

    // Update the number of bytes written
    my_context->written += bytes_to_write;

    return bytes_to_write;
}

static size_t dump_file_part_callback(const int img, int inode, void *context) {
    struct read_context *task_ctx = (struct read_context *) context;

    // Calculate the starting offset and size to read
    uint32_t offset = task_ctx->offset + task_ctx->written;
    uint32_t remaining = task_ctx->size - task_ctx->written;

    // Use `dump_ext2_file_part` to read the desired part of the file
    int ret = dump_ext2_file_part(img, inode, context, read_file_callback, offset, remaining);
    if (ret < 0) {
        return 0; // Signal an error or incomplete read
    }

    return task_ctx->written; // Return the total bytes written so far
}

static int ext2_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    printf("Reading %s\n", path);
    if ((fi->flags & O_ACCMODE) != O_RDONLY) { return -EACCES; }

    struct read_context task_ctx = {
        .buf = buf,
        .size = size,
        .offset = offset,
        .written = 0
    };

    // Use `dump_ext2_file_on_path` with `dump_file_part_callback`
    int ret = dump_ext2_file_on_path(ctx.fs->fd, path, &task_ctx, (on_file_dump_t) dump_file_part_callback);
    if (ret < 0) { return ret; }

    return (int) task_ctx.written;
}


static int ext2_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) path;
    (void) buf;
    (void) size;
    (void) offset;
    (void) fi;

    return -EROFS;
}

static int ext2_truncate(const char *path, off_t size, struct fuse_file_info *fi) {
    (void) path;
    (void) size;
    (void) fi;

    return -EROFS;
}

static int ext2_chmod(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void) path;
    (void) mode;
    (void) fi;

    return -EROFS;
}

static int ext2_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void) path;
    (void) mode;
    (void) fi;

    return -EROFS;
}


static const struct fuse_operations ext2_ops = {
    .getattr = ext2_getattr,
    .readdir = ext2_readdir,
    .open = ext2_open,
    .read = ext2_read,
    // And mock writes
    .write = ext2_write,
    .truncate = ext2_truncate,
    .chmod = ext2_chmod,
    .create = ext2_create
};

int ext2fuse(int img, const char *mntp) {
    // Initialize Ext2 filesystem
    int ret = ext2_fs_init(&ctx.fs, img);
    if (ret < 0) {
        return ret; // Return error if initialization fails
    }

    char *argv[] = {"ext2fuse", "-f", (char *) mntp, NULL};
    ret = fuse_main(3, argv, &ext2_ops, NULL);

    ext2_fs_free(ctx.fs);
    return ret;
}
