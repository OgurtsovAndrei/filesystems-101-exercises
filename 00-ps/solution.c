#include <solution.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

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
            char cmd_path[512], exe_path[512];
            char *argv[256], *envp[256];
            char env_path[512];
            char cmd_buffer[4096], env_buffer[8192];
            size_t arg_index = 0;
            size_t envc = 0;

            snprintf(exe_path, sizeof(exe_path), "/proc/%s/exe", entry->d_name);
            ssize_t exe_len = readlink(exe_path, cmd_buffer, sizeof(cmd_buffer) - 1);
            if (exe_len == -1) {
                report_error(exe_path, errno);
                continue;
            }
            cmd_buffer[exe_len] = '\0';

            snprintf(cmd_path, sizeof(cmd_path), "/proc/%s/cmdline", entry->d_name);
            cmdline_file = fopen(cmd_path, "r");
            if (cmdline_file == NULL) {
                report_error(cmd_path, errno);
                continue;
            }

            size_t read_bytes = fread(cmd_buffer, sizeof(char), sizeof(cmd_buffer), cmdline_file);
            fclose(cmdline_file);

            if (read_bytes > 0) {
                char *start = cmd_buffer;
                while (arg_index < 255 && start < cmd_buffer + read_bytes) {
                    argv[arg_index] = strdup(start);
                    if (argv[arg_index] == NULL) { break; }
                    start += strlen(start) + 1;
                    arg_index++;
                }
            }
            argv[arg_index] = NULL;

            snprintf(env_path, sizeof(env_path), "/proc/%s/environ", entry->d_name);
            environ_file = fopen(env_path, "r");
            if (environ_file == NULL) { goto cleanup; }

            read_bytes = fread(env_buffer, sizeof(char), sizeof(env_buffer), environ_file);
            fclose(environ_file);

            if (read_bytes > 0) {
                char *start = env_buffer;
                while (envc < 255 && start < env_buffer + read_bytes) {
                    envp[envc] = strdup(start);
                    if (envp[envc] == NULL) { break; }
                    start += strlen(start) + 1;
                    envc++;
                }
            }
            envp[envc] = NULL;

            report_process(pid, cmd_buffer, argv, envp);

        cleanup:
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
