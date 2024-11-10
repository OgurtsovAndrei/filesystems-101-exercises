//
// Created by andrei on 11/10/24.
//

#include "fs_void_vector.h"
#include "fs_dir_entry.h"

#include <fs_malloc.h>
#include <stdlib.h>
#include <string.h>
#include <asm-generic/errno-base.h>

#include "fs_ext2.h"

int init_dir_entry_from_buffer(const char *buffer,
                               size_t buffer_size,
                               uint64_t dir_entry_offset,
                               dir_entry **dir_entry_ptr) {
    dir_entry header;

    if (dir_entry_offset % 4 != 0) { return -EINVAL; }
    size_t header_size = sizeof(header.inode) +
                         sizeof(header.entry_size) +
                         sizeof(header.name_length) +
                         sizeof(header.type_indicator);

    if (dir_entry_offset + header_size > buffer_size) { return -EINVAL; }

    memcpy(&header, buffer + dir_entry_offset, header_size);

    size_t entry_size = header.entry_size;
    if (dir_entry_offset + entry_size > buffer_size || entry_size < header_size + header.name_length) return -EINVAL;

    dir_entry *entry = fs_xmalloc(entry_size);
    if (!entry) return -ENOMEM;

    memcpy(entry, buffer + dir_entry_offset, entry_size);

    *dir_entry_ptr = entry;
    return 0;
}

void free_entry(dir_entry *entry) { free(entry); }

char* get_name(dir_entry *entry) {
    char *name_copy = (char *) fs_xmalloc(entry->name_length + 1);
    memcpy(name_copy, entry->name, entry->name_length);
    name_copy[entry->name_length] = '\0';
    return name_copy;
}

int skip_entry(dir_entry *direntry) { return direntry->inode == 0; }


int is_dir(dir_entry *entry) {
    return entry->type_indicator == FS_IS_DIR;
}

int is_file(dir_entry *entry) {
    return entry->type_indicator == FS_IS_FILE;
}


int apply_on_entries(struct ext2_fs *file_system, struct ext2_blkiter *blkiter,
                     void *context, int (*lambda)(void *context, dir_entry *entry)) {
    int ret;
    char *buffer = fs_xmalloc(blkiter->block_size);
    uint32_t dir_size = blkiter->inode->size_lower;
    uint32_t bytes_read = 0;

    if (!is_inode_directory(blkiter->inode)) {
        ret = -1;
        goto cleanup;
    }

    while (bytes_read < dir_size) {
        int block_number = 0;
        int blkiter_next_res = ext2_blkiter_next(blkiter, &block_number);
        if (blkiter_next_res < 0) {
            ret = -blkiter_next_res;
            goto cleanup;
        }
        if (blkiter_next_res == 0) { break; }

        read_block(file_system->fd, blkiter->block_size, block_number, buffer);
        uint64_t offset = 0;

        while (offset < blkiter->block_size && bytes_read < dir_size) {
            dir_entry *entry;
            ret = init_dir_entry_from_buffer(buffer, blkiter->block_size, offset, &entry);
            if (ret < 0) { goto cleanup; }

            if (entry->entry_size % 4 != 0) {
                ret = -EINVAL;
                free_entry(entry);
                goto cleanup;
            }

            if (skip_entry(entry)) {
                offset += sizeof(entry->inode) + sizeof(entry->entry_size) + entry->entry_size;
                free_entry(entry);
                continue;
            }

            // Apply the lambda to this entry
            ret = lambda(context, entry);
            if (ret < 0) {
                free_entry(entry);
                goto cleanup;
            }

            offset += entry->entry_size;
            bytes_read += entry->entry_size;
        }
    }

    ret = 0;

    cleanup:
        free(buffer);
    return ret;
}

int add_entry_to_vector(void *context, dir_entry *entry) {
    fs_vector *vector = (fs_vector *) context;
    vector_add(vector, entry);
    return 0;
}

int dump_directory(struct ext2_fs *file_system, struct ext2_blkiter *blkiter, fs_vector *vector) {
    vector_init(vector);
    return apply_on_entries(file_system, blkiter, vector, add_entry_to_vector);
}