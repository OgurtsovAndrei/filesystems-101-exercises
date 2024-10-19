#ifndef FUSE_USE_VERSION
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
    pid_t pid = fuse_get_context()->pid;
    sprintf(result, "hello, %d\n", pid);
    return result;
}

static int hellofs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    (void) fi;
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
                           struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    (void) offset;
    (void) fi;
    (void) flags;

    struct stat st;
    memset(&st, 0, sizeof(st));

    if (strcmp(path, "/") != 0) {
        return -ENOENT;
    }

    st.st_mode = S_IFDIR | 0755;
    filler(buf, ".", &st, 0, 0);

    filler(buf, "..", &st, 0, 0);

    st.st_mode = S_IFREG | 0400;
    st.st_size = strlen(create_hello_string());
    filler(buf, "hello", &st, 0, 0);

    return 0;
}


static int hellofs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    if (strcmp(path, "/hello") != 0) return -ENOENT;
    if ((fi->flags & O_ACCMODE) != O_RDONLY) return -ENOENT;
    const char* content = create_hello_string();
    const int file_size = (int) strlen(content);
    if (offset == file_size) { return 0; }
    if (offset > file_size) { return 0; }
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
