#include "solution.h"

#include <err.h>
#include <fs_malloc.h>

#include "fs_ext2.h"
#include "fs_inode.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

int dump_file(int img, int inode_nr, int out) {
    struct ext2_fs *fs = NULL;
    struct ext2_blkiter *i = NULL;
    int ret;

    if ((ret = ext2_fs_init(&fs, img))) goto cleanup_no_buff;
    if ((ret = ext2_blkiter_init(&i, fs, inode_nr))) goto cleanup_no_buff;
    char *buffer = fs_xmalloc(i->block_size);

    uint32_t file_size = i->inode->size_lower;
    uint32_t bytes_read = 0;

    for (;;) {
        int blkno;
        ret = ext2_blkiter_next(i, &blkno);
        // printf("blkno = %d, ret = %i\n", blkno, ret);
        read_block(img, i->block_size, blkno, buffer);
        if (ret < 0) goto cleanup;

        if (ret > 0) {
            const u_int32_t bytes_written = pwrite(out, buffer, i->block_size, bytes_read);
            if (bytes_written != i->block_size) {
                ret = -EIO;
                goto cleanup;
            }
            // print_extra_data(buffer, i->block_size);
            // printf("Buffer content:\n%.*s\n", i->block_size, buffer);
            bytes_read += i->block_size;
        } else {
            const u_int32_t bytes_to_write = file_size - bytes_read;
            const u_int32_t bytes_written = pwrite(out, buffer, bytes_to_write, bytes_read);
            if (bytes_written != bytes_to_write) {
                ret = -EIO;
                goto cleanup;
            }
            // print_extra_data(buffer, file_size - bytes_read);
            // printf("Buffer content:\n%.*s\n", file_size - bytes_read, buffer);
            break;
        }
    }

    free(buffer);
    ext2_blkiter_free(i);
    ext2_fs_free(fs);
    return 0;

cleanup:
    free(buffer);
cleanup_no_buff:
    ext2_blkiter_free(i);
    ext2_fs_free(fs);
    return ret;
}
