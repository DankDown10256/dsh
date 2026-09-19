#define _XOPEN_SOURCE 700
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>
#include "py_venv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>

static char saved_path[4096];
static int venv_active = 0;

int is_python_venv(char *out_path, size_t out_size) {
    char *candidates[] = {"venv", ".venv", "env"};
    struct stat st;
    for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
        if (stat(candidates[i], &st) == 0 && S_ISDIR(st.st_mode)) {
            snprintf(out_path, out_size, "%s", candidates[i]);
            return 1;
        }
    }
    return 0;
}

int activate_python_venv(const char *venv_path) {
    char abs_venv[PATH_MAX];
    if (!realpath(venv_path, abs_venv)) {
        perror("realpath");
        return 0;
    }

    const char *old_path = getenv("PATH");
    if (old_path) {
        snprintf(saved_path, sizeof(saved_path), "%s", old_path);
    }

    setenv("VIRTUAL_ENV", abs_venv, 1);
    printf("Python venv successfully activated!\n");

    char new_path[4096];
    snprintf(new_path, sizeof(new_path), "%s/bin:%s", abs_venv, old_path ? old_path : "");
    setenv("PATH", new_path, 1);

    unsetenv("PYTHONHOME");
    venv_active = 1;

    return 1;
}

int deactivate_python_venv(void) {
    if (!venv_active) {
        printf("No active venv\n");
        return 0;
    }

    setenv("PATH", saved_path, 1);
    unsetenv("VIRTUAL_ENV");
    printf("Python venv successfully deactivated!\n");
    venv_active = 0;

    return 1;
}
