//
// Created by andrei on 11/10/24.
//


#ifndef FS_DIR_ENTRY_H
#define FS_DIR_ENTRY_H

#include <stdint.h>
#include <unistd.h>

#define	SKIP_ENTRY 1
#define	FS_IS_FILE 1
#define	FS_IS_DIR 2

#pragma pack(push, 1)

typedef struct {
    uint32_t inode; // Byte 0-3: Inode number
    uint16_t entry_size; // Byte 4-5: Total size of this entry (Including all subfields)
    uint8_t name_length; // Byte 6: Name length (least-significant 8 bits)
    uint8_t type_indicator; // Byte 7: Type indicator (or most-significant 8 bits of Name Length)
    char name[]; // Byte 8 to 8+N-1: Name characters (flexible array)
} dir_entry;

#pragma pack(pop)

int skip_entry(dir_entry *direntry);

int init_dir_entry_from_buffer(
    const char *buffer,
    size_t buffer_size,
    uint64_t dir_entry_offset,
    dir_entry **dir_entry_ptr
);

void free_entry(dir_entry *entry);

int is_file(dir_entry *entry);

int is_dir(dir_entry *entry);


#endif //FS_DIR_ENTRY_H
