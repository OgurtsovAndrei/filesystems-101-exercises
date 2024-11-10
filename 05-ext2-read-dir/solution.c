#include "solution.h"

#include <fs_malloc.h>
#include <err.h>
#include "fs_ext2.h"
#include "fs_inode.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "fs_dir_entry.h"


int dump_dir(int img, int inode_nr) {
    struct ext2_fs *file_system = NULL;
    struct ext2_blkiter *blkiter = NULL;
    int ret;

    if ((ret = ext2_fs_init(&file_system, img))) goto cleanup_no_buff;
    if ((ret = ext2_blkiter_init(&blkiter, file_system, inode_nr))) goto cleanup_no_buff;
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

            char name[entry->name_length + 1];
            if (skip_entry(entry)) {
                offset += sizeof(entry->inode) + sizeof(entry->entry_size) + entry->entry_size;
                goto entry_cleanup;
            }

            // printf("Inode: %u, Name: %.*s\n", entry->inode, entry->name_length, entry->name);
            memcpy(name, entry->name, entry->name_length);
            name[entry->name_length] = '\0';
            if (is_file(entry)) report_file((int) entry->inode, 'f', name);
            if (is_dir(entry)) report_file((int) entry->inode, 'd', name);

            offset += entry->entry_size;
            bytes_read += entry->entry_size;

        entry_cleanup:
            free_entry(entry);
        }
    }

    ret = 0;

cleanup:
    free(buffer);
cleanup_no_buff:
    ext2_blkiter_free(blkiter);
    ext2_fs_free(file_system);
    return ret;
}
