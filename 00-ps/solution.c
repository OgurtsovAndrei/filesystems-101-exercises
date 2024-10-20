#include <solution.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#define INITIAL_BUFFER_SIZE 4096

void ps(void) {
    struct dirent *entry;
    DIR *dp = opendir("/proc");

    if (dp == NULL) {
        report_error("/proc", errno);
        return;
    }

    while ((entry = readdir(dp))) {
        if (entry->d_type == DT_DIR && atoi(entry->d_name) > 0) {
            pid_t pid = atoi(entry->d_name);
            FILE *cmdline_file, *environ_file;
            char *cmd_buffer = NULL, *env_buffer = NULL;
            char exe_path[512], env_path[512], cmd_path[512];
            char exe_buffer[4096];
            char *argv[1024], *envp[1024];
            size_t arg_index = 0;
            size_t envc = 0;
            size_t cmd_buffer_size = INITIAL_BUFFER_SIZE;
            size_t env_buffer_size = INITIAL_BUFFER_SIZE;

            snprintf(exe_path, sizeof(exe_path), "/proc/%s/exe", entry->d_name);
            ssize_t exe_len = readlink(exe_path, exe_buffer, sizeof(exe_buffer) - 1);
            if (exe_len == -1) {
                report_error(exe_path, errno);
                continue;
            }
            exe_buffer[exe_len] = '\0';

            snprintf(cmd_path, sizeof(cmd_path), "/proc/%s/cmdline", entry->d_name);
            cmdline_file = fopen(cmd_path, "r");
            if (cmdline_file == NULL) {
                report_error(cmd_path, errno);
                continue;
            }

            cmd_buffer = (char *) malloc(cmd_buffer_size);
            if (cmd_buffer == NULL) {
                fclose(cmdline_file);
                continue;
            }

            size_t read_bytes = 0;
            size_t total_read_bytes = 0;
            while ((read_bytes = fread(cmd_buffer + total_read_bytes, 1, cmd_buffer_size - total_read_bytes,
                                       cmdline_file)) > 0) {
                total_read_bytes += read_bytes;
                if (total_read_bytes == cmd_buffer_size) {
                    cmd_buffer_size *= 2;
                    cmd_buffer = (char *) realloc(cmd_buffer, cmd_buffer_size);
                    if (cmd_buffer == NULL) {
                        fclose(cmdline_file);
                        goto cleanup;
                    }
                }
            }
            fclose(cmdline_file);

            if (total_read_bytes > 0) {
                char *start = cmd_buffer;
                char *end = cmd_buffer + total_read_bytes;

                while (arg_index < 1023 && start < end) {
                    if (*start != '\0') {
                        argv[arg_index] = strdup(start);
                        if (argv[arg_index] == NULL) { break; }
                        arg_index++;
                        start += strlen(start) + 1;
                    } else {
                        start++;
                    }
                }
                if (arg_index == 1024) { exit(-3); }
            }
            argv[arg_index] = NULL;

            snprintf(env_path, sizeof(env_path), "/proc/%s/environ", entry->d_name);
            environ_file = fopen(env_path, "r");
            if (environ_file == NULL) { goto cleanup; }

            env_buffer = (char *) malloc(env_buffer_size);
            if (env_buffer == NULL) {
                fclose(environ_file);
                goto cleanup;
            }

            read_bytes = 0;
            total_read_bytes = 0;
            while ((read_bytes = fread(env_buffer + total_read_bytes, 1, env_buffer_size - total_read_bytes,
                                       environ_file)) > 0) {
                total_read_bytes += read_bytes;
                if (total_read_bytes == env_buffer_size) {
                    env_buffer_size *= 2;
                    env_buffer = (char *) realloc(env_buffer, env_buffer_size);
                    if (env_buffer == NULL) {
                        fclose(environ_file);
                        goto cleanup;
                    }
                }
            }
            fclose(environ_file);

            if (total_read_bytes > 0) {
                char *start = env_buffer;
                while (envc < 1023 && start < env_buffer + total_read_bytes) {
                    envp[envc] = strdup(start);
                    if (envp[envc] == NULL) { break; }
                    start += strlen(start) + 1;
                    envc++;
                }
                if (envc == 1024) { exit(-3); }
            }
            envp[envc] = NULL;

            report_process(pid, exe_buffer, argv, envp);

        cleanup:
            if (cmd_buffer) free(cmd_buffer);
            if (env_buffer) free(env_buffer);
            for (size_t j = 0; j < arg_index; j++) {
                free(argv[j]);
            }
            for (size_t j = 0; j < envc; j++) {
                free(envp[j]);
            }
        }
    }

    closedir(dp);
}
