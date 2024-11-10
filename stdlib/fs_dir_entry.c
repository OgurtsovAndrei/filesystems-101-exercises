//
// Created by andrei on 11/10/24.
//

#include "fs_dir_entry.h"

#include <fs_malloc.h>
#include <stdlib.h>
#include <string.h>
#include <asm-generic/errno-base.h>

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

int skip_entry(dir_entry *direntry) { return direntry->inode == 0; }


int is_dir(dir_entry *entry) {
    return entry->type_indicator == FS_IS_DIR;
}

int is_file(dir_entry *entry) {
    return entry->type_indicator == FS_IS_FILE;
}
