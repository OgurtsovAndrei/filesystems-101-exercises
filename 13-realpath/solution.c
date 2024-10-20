#include <solution.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAX_PATH_BUFFER_SIZE 4096

void abspath(const char *path) {
    char resolved_path[MAX_PATH_BUFFER_SIZE];
    char symlink_target[MAX_PATH_BUFFER_SIZE];
    char initial_path[MAX_PATH_BUFFER_SIZE];
    char *path_to_resolve = initial_path;
    struct stat path_stat;

    resolved_path[0] = '\0';

    if (path[0] != '/') {
        snprintf(initial_path, sizeof(initial_path), "/%s", path);
    } else {
        snprintf(initial_path, sizeof(initial_path), "%s", path);
    }

    while (path_to_resolve != NULL) {
        char current_segment[MAX_PATH_BUFFER_SIZE];
        char *next = strchr(path_to_resolve + 1, '/');
        size_t current_segment_size;
        if (next != NULL) {
            current_segment_size = next - path_to_resolve;
        } else {
            current_segment_size = strlen(path_to_resolve);
        }
        memcpy(current_segment, path_to_resolve, current_segment_size);
        current_segment[current_segment_size] = '\0';
        // printf("%s\n", current_segment);
        if (strcmp(current_segment, "/.") == 0) goto next;
        if (strcmp(current_segment, "/..") == 0) {
            if (*(resolved_path + strlen(resolved_path) - 1) == '/') *(resolved_path + strlen(resolved_path) - 1) = '\0';
            char *last_slash = strrchr(resolved_path, '/');
            if (last_slash != NULL) {
                *last_slash = '\0';
            }
            // printf("path now %s\n", resolved_path);
            goto next;
        }

        if (resolved_path[0] != '\0' && resolved_path[strlen(resolved_path) - 1] == '/' && current_segment[0] == '/') {
            snprintf(resolved_path + strlen(resolved_path), sizeof(resolved_path) - strlen(resolved_path), "%s", current_segment + 1);
        } else {
            snprintf(resolved_path + strlen(resolved_path), sizeof(resolved_path) - strlen(resolved_path), "%s", current_segment);
        }

        const ssize_t len = readlink(resolved_path, symlink_target, MAX_PATH_BUFFER_SIZE - 1);
        if (len != -1) {
            symlink_target[len] = '\0';
            if (symlink_target[0] == '/') {
                snprintf(resolved_path, sizeof(resolved_path), "%s", symlink_target);
            } else {
                exit(-2);
            }
        }

        if (stat(resolved_path, &path_stat) == -1) {
            report_error(resolved_path, path_to_resolve, errno);
            return;
        }

        if (S_ISDIR(path_stat.st_mode) && resolved_path[strlen(resolved_path) - 1] != '/') {
            snprintf(resolved_path + strlen(resolved_path), sizeof(resolved_path) - strlen(resolved_path), "/");
        }

    next:
        path_to_resolve = next;
    }

    report_path(resolved_path);
}
