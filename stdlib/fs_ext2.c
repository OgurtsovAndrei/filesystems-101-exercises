#include "fs_malloc.h"
#include "fs_ext2.h"
#include "fs_inode.h"
#include "fs_superblock.h"
#include "fs_block_group_descriptor_table.h"
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "fs_dir_entry.h"
#include "fs_void_vector.h"

void parse_path_to_segments(const char *path, fs_vector *segments) {
    char initial_path[MAX_PATH_BUFFER_SIZE];
    if (path[0] != '/') {
        snprintf(initial_path, sizeof(initial_path), "/%s", path);
    } else {
        snprintf(initial_path, sizeof(initial_path), "%s", path);
    }

    char *path_to_resolve = initial_path + 1;
    while (*path_to_resolve != '\0') {
        char *next = strchr(path_to_resolve + 1, '/');
        size_t segment_len;
        if (next != NULL) {
            segment_len = next - path_to_resolve;
        } else {
            segment_len = strlen(path_to_resolve);
        }

        fs_string *segment = (fs_string *) fs_xmalloc(sizeof(fs_string));
        fs_string_init(segment);
        fs_string_reserve(segment, segment_len);
        memcpy(segment->data, path_to_resolve, segment_len);
        segment->length = segment_len;

        if (segment->length == 1 && memcmp(segment->data, ".", segment->length) == 0) {
            fs_string_free(segment);
        }

        if (segment->length == 2 && memcmp(segment->data, "..", segment->length) == 0) {
            fs_string *previous = vector_pop_or_null(segments);
            fs_string_free(previous);
            fs_string_free(segment);
        }

        vector_add(segments, segment);
        path_to_resolve = next ? next + 1 : path_to_resolve + segment_len;
    }
}


int ext2_fs_init(struct ext2_fs **fs, int fd) {
    int ret;
    fs_superblock *superblock = NULL;
    struct ext2_fs *file_system = NULL;

    superblock = fs_xmalloc(sizeof(fs_superblock));
    ret = init_superblock(fd, superblock);
    if (ret != 0) goto clenup;

    file_system = fs_xmalloc(sizeof(struct ext2_fs));
    file_system->fd = fd;
    file_system->superblock = superblock;
    *fs = file_system;
    return 0;

clenup:
    free(superblock);
    free(file_system);
    return ret;
}

void ext2_fs_free(struct ext2_fs *fs) {
    if (fs == NULL) return;
    fs_superblock *superblock = fs->superblock;
    free(superblock);
    close(fs->fd);
    free(fs);
}

int ext2_blkiter_init(struct ext2_blkiter **i, struct ext2_fs *fs, int ino) {
    fs_blockgroup_descriptor *blockgroup_descriptor = fs_xmalloc(sizeof(fs_blockgroup_descriptor));;
    u_int32_t block_size = 1 << (10 + fs->superblock->s_log_block_size_kbytes);
    struct ext2_blkiter *blkiter = fs_xmalloc(sizeof(struct ext2_blkiter) + 3 * block_size);
    blkiter->iterator_block_index = 0;
    blkiter->block_size = block_size;
    blkiter->single_indirect_block_cache_id = -1;
    blkiter->double_indirect_block_cache_id = -1;
    blkiter->triple_indirect_block_cache_id = -1;
    blkiter->blockgroup_descriptor = blockgroup_descriptor;


    uint32_t block_group = (ino - 1) / fs->superblock->s_inodes_per_group;

    int ret = init_blockgroup_descriptor(fs->fd, fs->superblock, blockgroup_descriptor, block_group);
    if (ret != 0) goto cleenup;

    ret = init_inode(fs->fd, fs->superblock, blockgroup_descriptor, ino, &blkiter->inode);
    // print_inode_info(blkiter->inode, fs->superblock->s_inode_size, ino, blkiter->block_size, fs->fd);
    if (ret != 0) goto cleenup;

    blkiter->fd = fs->fd;
    *i = blkiter;
    return 0;

cleenup:
    free(blockgroup_descriptor);
    free(blkiter);
    return ret;
}

int ext2_blkiter_next(struct ext2_blkiter *i, int *blkno) {
    uint32_t block_number = 0;

    int ret = get_inode_block_address_by_index(i->fd, i->iterator_block_index++, &block_number, i);
    if (ret == -ERANGE) return 0;
    if (ret < 0) { return ret; }
    *blkno = (int) block_number;
    return 1;
}

void ext2_blkiter_free(struct ext2_blkiter *i) {
    if (i == NULL) return;
    free(i->inode);
    free(i->blockgroup_descriptor);
    free(i);
}

int get_inode_block_address_by_index(int fd, uint32_t inode_index,
                                     uint32_t *block_address_ptr, struct ext2_blkiter *bllkiter) {
    uint32_t block_size = bllkiter->block_size;
    uint32_t used_blocks = (bllkiter->inode->size_lower + block_size - 1) / block_size;
    if (inode_index >= used_blocks) return -ERANGE;

    uint32_t block_address;

    if (inode_index < DIRECT_POINTERS) {
        // Direct block
        block_address = bllkiter->inode->direct_block_pointers[inode_index];
        goto end_get_id;
    }

    char *cache = bllkiter->indirect_pointer_cache;
    char *single_cache = cache;              // Block 1
    char *double_cache = cache + block_size; // Block 2
    char *triple_cache = cache + 2 * block_size; // Block 3

    if (inode_index < (DIRECT_POINTERS + block_size / BLOCK_ADDRESS_SIZE)) {
        // Single indirect block
        uint32_t singly_offset = inode_index - DIRECT_POINTERS;

        if (bllkiter->single_indirect_block_cache_id != (int64_t) bllkiter->inode->singly_indirect_block) {
            int ret = read_block(fd, block_size, bllkiter->inode->singly_indirect_block, single_cache);
            if (ret) return ret;

            bllkiter->single_indirect_block_cache_id = (int64_t) bllkiter->inode->singly_indirect_block;
        }

        block_address = ((uint32_t *) single_cache)[singly_offset];
        goto end_get_id;
    }

    if (inode_index < DIRECT_POINTERS + block_size / BLOCK_ADDRESS_SIZE + block_size / BLOCK_ADDRESS_SIZE * (block_size / BLOCK_ADDRESS_SIZE)) {
        // Double indirect block
        uint32_t doubly_offset = inode_index - (DIRECT_POINTERS + block_size / BLOCK_ADDRESS_SIZE);
        uint32_t singly_index = doubly_offset / (block_size / BLOCK_ADDRESS_SIZE);
        uint32_t indirect_index = doubly_offset % (block_size / BLOCK_ADDRESS_SIZE);

        if (bllkiter->double_indirect_block_cache_id != (int64_t) bllkiter->inode->doubly_indirect_block) {
            int ret = read_block(fd, block_size, bllkiter->inode->doubly_indirect_block, double_cache);
            if (ret) return ret;

            bllkiter->double_indirect_block_cache_id = (int64_t) bllkiter->inode->doubly_indirect_block;
        }

        uint32_t singly_indirect_block_address = ((uint32_t *) double_cache)[singly_index];

        if (bllkiter->single_indirect_block_cache_id != (int64_t) singly_indirect_block_address) {
            int ret = read_block(fd, block_size, singly_indirect_block_address, single_cache);
            if (ret) return ret;

            bllkiter->single_indirect_block_cache_id = (int64_t) singly_indirect_block_address;
        }

        block_address = ((uint32_t *) single_cache)[indirect_index];
        goto end_get_id;
    } {
        // Triple indirect block
        uint32_t triply_offset = inode_index - (DIRECT_POINTERS + block_size / BLOCK_ADDRESS_SIZE +
                                                (block_size / BLOCK_ADDRESS_SIZE) * (block_size / BLOCK_ADDRESS_SIZE));
        uint32_t doubly_index = triply_offset / ((block_size / BLOCK_ADDRESS_SIZE) * (block_size / BLOCK_ADDRESS_SIZE));
        uint32_t singly_index = (triply_offset / (block_size / BLOCK_ADDRESS_SIZE)) % (block_size / BLOCK_ADDRESS_SIZE);
        uint32_t indirect_index = triply_offset % (block_size / BLOCK_ADDRESS_SIZE);

        if (bllkiter->triple_indirect_block_cache_id != (int64_t) bllkiter->inode->triply_indirect_block) {
            int ret = read_block(fd, block_size, bllkiter->inode->triply_indirect_block, triple_cache);
            if (ret) return ret;

            bllkiter->triple_indirect_block_cache_id = (int64_t) bllkiter->inode->triply_indirect_block;
        }

        uint32_t double_indirect_block_address = ((uint32_t *) triple_cache)[doubly_index];

        if (bllkiter->double_indirect_block_cache_id != (int64_t) double_indirect_block_address) {
            int ret = read_block(fd, block_size, double_indirect_block_address, double_cache);
            if (ret) return ret;

            bllkiter->double_indirect_block_cache_id = (int64_t) double_indirect_block_address;
        }

        uint32_t singly_indirect_block_address = ((uint32_t *) double_cache)[singly_index];

        if (bllkiter->single_indirect_block_cache_id != (int64_t) singly_indirect_block_address) {
            int ret = read_block(fd, block_size, singly_indirect_block_address, single_cache);
            if (ret) return ret;

            bllkiter->single_indirect_block_cache_id = (int64_t) singly_indirect_block_address;
        }

        block_address = ((uint32_t *) single_cache)[indirect_index];
    }

end_get_id:
    *block_address_ptr = block_address;
    if (inode_index == used_blocks - 1) return 0;
    return 1;
}


int dump_ext2_file(int img, int inode_nr, int out) {
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
            u_int32_t bytes_to_write = file_size - bytes_read;
            if (bytes_to_write > i->block_size) { bytes_to_write = i->block_size; }
            const u_int32_t bytes_written = pwrite(out, buffer, bytes_to_write, bytes_read);
            if (bytes_written != bytes_to_write) {
                ret = -EIO;
                goto cleanup;
            }
            bytes_read += bytes_to_write;
            // print_extra_data(buffer, i->block_size);
            // printf("Buffer content:\n%.*s\n", i->block_size, buffer);
        } else /* ret == 0 */  {
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


int dump_ext2_file_on_path(int img, const char *path, int out) {
    int ret = 0;
    struct ext2_fs *file_system = NULL;
    struct ext2_blkiter *blkiter = NULL;
    if ((ret = ext2_fs_init(&file_system, img))) goto cleanup;

    fs_vector path_segments;
    vector_init(&path_segments);
    parse_path_to_segments(path, &path_segments);

    VectorIterator iterator;
    vector_iterator_init(&iterator, &path_segments);

    int current_inode = ROOT_INODE_ID;

    while (vector_iterator_has_next(&iterator) && current_inode != 0) {
        fs_string *segment = vector_iterator_next(&iterator);
        // printf("Look for:  %.*s\n", (int) segment->length, segment->data);

        if ((ret = ext2_blkiter_init(&blkiter, file_system, current_inode))) {
            ret = -ENOENT;
            goto cleanup;
        }

        fs_vector dir_entries;
        dump_directory(file_system, blkiter, &dir_entries);

        uint32_t segment_inode = 0;
        VectorIterator dir_entries_iterator;
        vector_iterator_init(&dir_entries_iterator, &dir_entries);
        // printf("Entries:\n");
        while (vector_iterator_has_next(&dir_entries_iterator)) {
            dir_entry *entry = (dir_entry *) vector_iterator_next(&dir_entries_iterator);
            if (iterator.current == iterator.vector->size) {
                // have to be a file
                char *file_name = get_name(entry);
                // printf("File name: %s\n", file_name);
                if (strlen(file_name) != segment->length || 0 != memcmp(file_name, segment->data, segment->length)) {
                    free(file_name);
                    continue;
                }
                free(file_name);
                if (!is_file(entry)) {
                    ret = -EISDIR;
                    goto clean_segment;
                }

                ret = dump_ext2_file(img, (int) entry->inode, out);
                goto clean_segment;
            }
            // have to be directory
            char *dir_name = get_name(entry);
            // printf("Dir name: %s\n", dir_name);
            size_t dir_name_len = strlen(dir_name);
            if (dir_name_len != segment->length || 0 != strncmp(dir_name, segment->data, dir_name_len)) {
                free(dir_name);
                continue;
            }
            free(dir_name);
            if (!is_dir(entry)) {
                ret = -ENOTDIR;
                goto clean_segment;
            }

            segment_inode = entry->inode;
            break;
        }

        if (segment_inode == 0) {
            ret = -ENOENT;
        }

    clean_segment:
        // printf("segment Cleanup\n");
        current_inode = (int) segment_inode;
        vector_free_all_elements(&dir_entries, (void (*)(void *)) free_entry);
        vector_free(&dir_entries);
        ext2_blkiter_free(blkiter);
    }


cleanup:
    // printf("Cleanup\n");
    vector_free_all_elements(&path_segments, (void (*)(void *)) fs_string_free);
    vector_free(&path_segments);
    ext2_fs_free(file_system);
    return ret;
}