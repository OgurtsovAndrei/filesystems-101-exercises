#ifndef USE_USE_VERSION
#define FUSE_USE_VERSION 31
#endif


#include <fuse.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

static char *create_hello_string() {
    static char result[50];
    sprintf(result, "hello, %d\n", getpid());
    return result;
}

static int hellofs_getattr(const char *path, struct stat *stbuf) {
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        stbuf->st_size = 0;
        stbuf->st_uid = geteuid();
        stbuf->st_gid = getegid();
    } else if (strcmp(path, "/hello") == 0) {
        stbuf->st_mode = S_IFREG | 0400;
        stbuf->st_nlink = 1;
        stbuf->st_size = (long) strlen(create_hello_string());
        stbuf->st_uid = geteuid();
        stbuf->st_gid = getegid();
    } else { return -ENOENT; }

    return 0;
}


static int hellofs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
                           struct fuse_file_info *fi) {
    (void) fi;
    (void) offset;

    if (strcmp(path, "/") != 0) { return -ENOENT; }
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    filler(buf, "hello", NULL, 0);
    return 0;
}


static int hellofs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    if (strcmp(path, "/hello") != 0) return -ENOENT;
    if ((fi->flags & O_ACCMODE) != O_RDONLY) return -ENOENT;
    const char* content = create_hello_string();
    const int file_size = (int) strlen(content);
    if (offset == file_size) { return 0; }
    if (offset > file_size) { return EOF; }
    if (size > file_size - (size_t) offset){ size = file_size - offset; }
    memcpy(buf, content + offset, size);
    return (int) size;
}

static struct fuse_operations hellofs_ops = {
    .getattr = hellofs_getattr,
    .read = hellofs_read,
    .readdir = hellofs_readdir,
};

int helloworld(const char *mntp) {
    char *argv[] = {"exercise", "-f", (char *) mntp, NULL};
    return fuse_main(3, argv, &hellofs_ops, NULL);
}
