#include <solution.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#define INITIAL_BUFFER_SIZE 512
#define MAX_PATH_BUFFER_SIZE 4096

void lsof(void) {
    struct dirent *entry;
    DIR *dp = opendir("/proc");

    if (dp == NULL) {
        report_error("/proc", errno);
        return;
    }

    while ((entry = readdir(dp))) {
        if (entry->d_type == DT_DIR && atoi(entry->d_name) > 0) {
            pid_t pid = atoi(entry->d_name);
            char *fd_path = NULL;
            char *link_path = NULL;
            char *file_path = NULL;
            DIR *fd_dir = NULL;

            fd_path = (char *) malloc(INITIAL_BUFFER_SIZE);
            if (fd_path == NULL) { exit(2); }

            snprintf(fd_path, INITIAL_BUFFER_SIZE, "/proc/%d/fd", pid);
            fd_dir = opendir(fd_path);

            if (fd_dir == NULL) {
                report_error(fd_path, errno);
                goto loop_cleanup;
            }

            while ((entry = readdir(fd_dir))) {
                if (entry->d_type == DT_LNK) {
                    link_path = (char *) malloc(INITIAL_BUFFER_SIZE);
                    if (link_path == NULL) { exit(2); }
                    file_path = (char *) malloc(MAX_PATH_BUFFER_SIZE);
                    if (file_path == NULL) { exit(2); }


                    snprintf(link_path, INITIAL_BUFFER_SIZE, "%s/%s", fd_path, entry->d_name);

                    ssize_t len = readlink(link_path, file_path, MAX_PATH_BUFFER_SIZE - 1);
                    if (len == -1) {
                        report_error(link_path, errno);
                        goto loop_cleanup;
                    }

                    file_path[len] = '\0';

                    report_file(file_path);

                    free(link_path), link_path = NULL;
                    free(file_path), file_path = NULL;
                }
            }

        loop_cleanup:
            if (fd_dir != NULL) { closedir(fd_dir); }
            if (fd_path != NULL) { free(fd_path); }
            if (link_path != NULL) { free(link_path); }
            if (file_path != NULL) { free(file_path); }
        }
    }

    closedir(dp);
}
