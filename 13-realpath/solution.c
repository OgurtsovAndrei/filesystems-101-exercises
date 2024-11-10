#include "solution.h"
#include "fs_void_vector.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fs_malloc.h>
#include <limits.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>

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

        vector_add(segments, segment);
        path_to_resolve = next ? next + 1 : path_to_resolve + segment_len;
    }
}

void abspath(const char *path) {
    fs_vector resolved_path_segments;
    vector_init(&resolved_path_segments);

    char symlink_target[MAX_PATH_BUFFER_SIZE];
    fs_vector path_segments;
    vector_init(&path_segments);
    struct stat path_stat;

    // Split the input path into segments and add them to the vector
    parse_path_to_segments(path, &path_segments);

    VectorIterator iterator;
    vector_iterator_init(&iterator, &path_segments);

    while (vector_iterator_has_next(&iterator)) {
        fs_string *segment = vector_iterator_next(&iterator);
        printf("%.*s\n", (int) segment->length, segment->data);
    }

    vector_iterator_init(&iterator, &path_segments);
    while (vector_iterator_has_next(&iterator)) {
        fs_string *segment = vector_iterator_next(&iterator);

        // Handle "." segment
        if (strcmp(segment->data, ".") == 0) {
            fs_string_free(segment);
            continue;
        }

        // Handle ".." segment
        if (strcmp(segment->data, "..") == 0) {
            fs_string_free(segment);
            if (resolved_path_segments.size > 0) {
                fs_string *str = (fs_string *) vector_pop_or_null(&resolved_path_segments);
                fs_string_free(str);
                free(str);
            }
            continue;
        }

        // Append the current segment to the resolved path segments
        fs_string *copy = fs_string_duplicate(segment);
        vector_add(&resolved_path_segments, copy);

        // Handle symlinks
        fs_string resolved_path_str;
        fs_string_init(&resolved_path_str);
        for (size_t i = 0; i < resolved_path_segments.size; ++i) {
            fs_string *sym_segment = (fs_string *) resolved_path_segments.data[i];
            fs_string_append(&resolved_path_str, "/");
            fs_string_append(&resolved_path_str, sym_segment->data);
        }

        char *resolved_path_cstr = fs_string_to_cstr(&resolved_path_str);
        const ssize_t len = readlink(resolved_path_cstr, symlink_target, MAX_PATH_BUFFER_SIZE - 1);
        free(resolved_path_cstr);
        fs_string_free(&resolved_path_str);

        if (len != -1) {
            symlink_target[len] = '\0';
            vector_free_all_elements(&resolved_path_segments, (void (*)(void *)) fs_string_free);
            resolved_path_segments.size = 0;
            vector_init(&resolved_path_segments);

            // Split symlink target into segments and add to resolved path
            parse_path_to_segments(symlink_target, &resolved_path_segments);
            vector_iterator_init(&iterator, &path_segments); // Restart iteration after resolving symlink
        }
    }

    // Check if the current path segment exists
    fs_string resolved_path_check;
    fs_string_init(&resolved_path_check);
    for (size_t i = 0; i < resolved_path_segments.size; ++i) {
        fs_string *segment = (fs_string *) resolved_path_segments.data[i];
        fs_string_append(&resolved_path_check, "/");
        fs_string_append(&resolved_path_check, segment->data);
    }

    char *resolved_path_cstr_check = fs_string_to_cstr(&resolved_path_check);
    if (stat(resolved_path_cstr_check, &path_stat) == -1) {
        report_error(resolved_path_cstr_check, "", errno);
        free(resolved_path_cstr_check);
        fs_string_free(&resolved_path_check);
        goto cleanup;
    }
    free(resolved_path_cstr_check);
    fs_string_free(&resolved_path_check);

    // Create final resolved path string
    fs_string final_path_str;
    fs_string_init(&final_path_str);
    for (size_t i = 0; i < resolved_path_segments.size; ++i) {
        fs_string *segment = (fs_string *) resolved_path_segments.data[i];
        fs_string_append(&final_path_str, "/");
        fs_string_append(&final_path_str, segment->data);
    }

    char *final_path = fs_string_to_cstr(&final_path_str);
    report_path(final_path);
    free(final_path);
    fs_string_free(&final_path_str);

cleanup:
    vector_free_all_elements(&resolved_path_segments, (void (*)(void *)) fs_string_free);
    vector_free(&resolved_path_segments);
    vector_free_all_elements(&path_segments, (void (*)(void *)) fs_string_free);
    vector_free(&path_segments);
}
