#include "solution.h"

#include <fs_malloc.h>
#include <stdio.h>
#include <string.h>

#include "fs_dir_entry.h"
#include "fs_ext2.h"
#include "fs_void_vector.h"


int dump_file(int img, const char *path, int out) {
    return dump_ext2_file_on_path(img, path, &out, file_dump_callback);
}
