#include <solution.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>

#define INITIAL_BUFFER_SIZE 512
#define MAX_PATH_BUFFER_SIZE 4096

void abspath(const char *path) {
    char resolved_path[MAX_PATH_BUFFER_SIZE];
    char symlink_target[MAX_PATH_BUFFER_SIZE];
    char initial_path[MAX_PATH_BUFFER_SIZE];
    char *path_to_resolve = initial_path;
    struct stat path_stat;

    strcpy(resolved_path, "");

    if (path[0] != '/') {
        strcpy(path_to_resolve, "/");
        strcat(path_to_resolve, path);
    } else {
        strcpy(path_to_resolve, path);
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
        strcat(resolved_path, current_segment);


        const ssize_t len = readlink(resolved_path, symlink_target, MAX_PATH_BUFFER_SIZE - 1);
        if (len != -1) {
            symlink_target[len] = '\0';
            if (symlink_target[0] == '/') { strcpy(resolved_path, symlink_target); } else { exit(-2); }
        }

        if (stat(resolved_path, &path_stat) == -1) {
            report_error(resolved_path, path_to_resolve, errno);
            return;
        }

        if (S_ISDIR(path_stat.st_mode) && resolved_path[strlen(resolved_path) - 1] != '/') {
            strcat(resolved_path, "/");
        }

        path_to_resolve = next;
    }

    report_path(resolved_path);
}
