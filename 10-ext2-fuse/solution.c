#include "solution.h"
#include <fuse.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "fs_ext2.h"

struct ext2_context {
	struct ext2_fs *fs;
};

static struct ext2_context ctx;

static int ext2_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
	(void) fi;  // Unused for now
	memset(stbuf, 0, sizeof(struct stat));

	// Parse the path and fetch inode details
	// todo: Use ext2 functions to get the inode and its attributes

	return -ENOENT;  // Placeholder: inode not found
}

static int ext2_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
						struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
	(void) offset;
	(void) fi;
	(void) flags;

	// todo: Fetch directory entries and fill the buffer
	return -ENOENT;  // Placeholder: directory not found
}

static int ext2_open(const char *path, struct fuse_file_info *fi) {
	// todo: Check if the file exists and is accessible
	return -ENOENT;  // Placeholder: file not found
}

static int ext2_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	if ((fi->flags & O_ACCMODE) != O_RDONLY) return -ENOENT;

	int ret = dump_ext2_file_on_path(ctx.fs->fd, path, -1);  // Use temporary output buffer
	if (ret < 0) return ret;

	// Copy content to FUSE buffer (simulated)
	memcpy(buf, ctx.buffer + offset, size);
	return size;
}

static const struct fuse_operations ext2_ops = {
	.getattr = ext2_getattr,
	.readdir = ext2_readdir,
	.open = ext2_open,
	.read = ext2_read,
};

int ext2fuse(int img, const char *mntp) {
	// Initialize Ext2 filesystem
	int ret = ext2_fs_init(&ctx.fs, img);
	if (ret < 0) {
		return ret;  // Return error if initialization fails
	}

	char *argv[] = {"ext2fuse", "-f", (char *)mntp, NULL};
	ret = fuse_main(3, argv, &ext2_ops, NULL);

	ext2_fs_free(ctx.fs);
	return ret;
}
