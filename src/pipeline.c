#define _POSIX_C_SOURCE 200809L
#include "pipeline.h"
#include <stdio.h>
#include <string.h>

void run_pipeline(const char *cmd1, const char *cmd2) {
    FILE *fp1 = popen(cmd1, "r");
    if (!fp1) {
        perror("popen");
        return;
    }

    char buffer[4096];
    char full_output[65536] = {0};
    while (fgets(buffer, sizeof(buffer), fp1)) {
        strncat(full_output, buffer, sizeof(full_output) - strlen(full_output) - 1);
    }
    pclose(fp1);

    FILE *fp2 = popen(cmd2, "w");
    if (!fp2) {
        perror("popen");
        return;
    }
    fputs(full_output, fp2);
    pclose(fp2);
}
