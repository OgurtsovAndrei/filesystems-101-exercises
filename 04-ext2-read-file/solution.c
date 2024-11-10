#include "solution.h"

#include <err.h>
#include <fs_malloc.h>

#include "fs_ext2.h"
#include "fs_inode.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

int dump_file(int img, int inode_nr, int out) {
    return dump_ext2_file(img, inode_nr, out);
}
