#include "solution.h"

#include <fs_malloc.h>
#include <stdio.h>
#include <string.h>

#include "fs_dir_entry.h"
#include "fs_ext2.h"
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

const int ROOT_INODE_ID = 2;

int dump_file(int img, const char *path, int out) {
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
