#include <liburing.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include "fs_malloc.h"

#define BLOCK_SZ (256 * 1024)
#define MAX_INFLIGHT_READS 4
#define QUEUE_DEPTH (MAX_INFLIGHT_READS * 2)

#define CHECK_ERROR(cond, msg, ret_code, action) \
    do { \
        if ((cond)) { \
            fprintf(stderr, "%s\n", (msg)); \
            ret = (ret_code); \
            action; \
        } \
    } while (0)

// Taken from https://unixism.net/loti/low_level.html documentation
off_t get_file_size(int fd) {
    struct stat st;

    if (fstat(fd, &st) < 0) {
        perror("fstat");
        return -1;
    }
    if (S_ISBLK(st.st_mode)) {
        unsigned long long bytes;
        if (ioctl(fd, BLKGETSIZE64, &bytes) != 0) {
            perror("ioctl");
            return -1;
        }
        return bytes;
    } else if (S_ISREG(st.st_mode))
        return st.st_size;

    return -1;
}

int copy(int in, int out) {
    struct io_uring ring;
    struct io_uring_sqe *sqe;
    struct io_uring_cqe *cqe;
    char *read_buffers[MAX_INFLIGHT_READS];
    int ret = 0;
    int inflight_read_index = 0;
    int inflight_write_index = 0;
    off_t offset_in = 0;
    off_t offset_out = 0;

    if (io_uring_queue_init(QUEUE_DEPTH, &ring, 0) < 0) {
        perror("io_uring_queue_init");
        return 1;
    }

    off_t file_sz = get_file_size(in);
    CHECK_ERROR(file_sz < 0, "Failed to get file size", 1, goto cleanup);

    int blocks_count = (int) file_sz / BLOCK_SZ;
    if (file_sz % BLOCK_SZ) blocks_count++;

    for (int i = 0; i < MAX_INFLIGHT_READS; i++) {
        read_buffers[i] = fs_xmalloc(BLOCK_SZ);
    }

    // printf("File size: %ld\n", file_sz);
    // printf("Block count: %d\n", blocks_count);

    bool init_schedule_read = true;

    while (offset_in < file_sz || inflight_read_index > 0 || inflight_write_index > 0) {
        if (init_schedule_read) {
            init_schedule_read = false;
            while (inflight_read_index < MAX_INFLIGHT_READS && offset_in < file_sz) {
                sqe = io_uring_get_sqe(&ring);
                CHECK_ERROR(!sqe, "Failed to get submission queue entry", 1, goto cleanup);

                io_uring_prep_read(sqe, in, read_buffers[inflight_read_index], BLOCK_SZ, offset_in);
                sqe->user_data = inflight_read_index | ((uint64_t) 0 << 32);
                offset_in += BLOCK_SZ;
                inflight_read_index++;
            }
        }

        ret = io_uring_submit(&ring);
        CHECK_ERROR(ret < 0, "io_uring_submit", -errno, goto cleanup);

        ret = io_uring_wait_cqe(&ring, &cqe);
        CHECK_ERROR(ret < 0, "io_uring_wait_cqe", -errno, goto cleanup);

        if (cqe->res < 0) {
            fprintf(stderr, "I/O operation failed: %s\n", strerror(-cqe->res));
            ret = -cqe->res;
            io_uring_cqe_seen(&ring, cqe);
            goto cleanup;
        }

        int completed_index = cqe->user_data & 0xFFFFFFFF;
        int operation_type = (cqe->user_data >> 32) & 0xFFFFFFFF;

        if (operation_type == 0 /* read complete */) {
            if (offset_out < file_sz) {
                size_t write_size = (offset_out + BLOCK_SZ > file_sz) ? (file_sz - offset_out) : BLOCK_SZ;

                sqe = io_uring_get_sqe(&ring);
                CHECK_ERROR(!sqe, "Failed to get submission queue entry", 1, goto cleanup);

                io_uring_prep_write(sqe, out, read_buffers[completed_index], write_size, offset_out);
                sqe->user_data = completed_index | ((uint64_t) 1 << 32);
                offset_out += write_size;
            } else { inflight_write_index--; }
        } else if (operation_type == 1 /* write complete */) {
            memset(read_buffers[completed_index], 0, BLOCK_SZ);
            if (offset_in < file_sz) {
                sqe = io_uring_get_sqe(&ring);
                CHECK_ERROR(!sqe, "Failed to get submission queue entry", 1, goto cleanup);

                io_uring_prep_read(sqe, in, read_buffers[completed_index], BLOCK_SZ, offset_in);
                sqe->user_data = completed_index | ((uint64_t) 0 << 32);
                offset_in += BLOCK_SZ;
            } else { inflight_read_index--; }
        }

        io_uring_cqe_seen(&ring, cqe);
    }

cleanup:
    for (int i = 0; i < MAX_INFLIGHT_READS; i++) { free(read_buffers[i]); }
    if (in >= 0) { close(in); }
    if (out >= 0) { close(out); }
    io_uring_queue_exit(&ring);
    return ret;
}
