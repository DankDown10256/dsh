#include "git.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

int is_git_repo(void) {
    struct stat st;
    return stat(".git", &st) == 0 && S_ISDIR(st.st_mode);
}

int get_git_branch(char *out, size_t out_size) {
    FILE *f = fopen(".git/HEAD", "r");
    if (!f) return 0;

    char line[256];
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return 0;
    }
    fclose(f);

    line[strcspn(line, "\n")] = 0;

    const char *prefix = "ref: refs/heads/";
    size_t prefix_len = strlen(prefix);

    if (strncmp(line, prefix, prefix_len) == 0) {
        snprintf(out, out_size, "%s", line + prefix_len);
    } else {
        char short_hash[8];
        strncpy(short_hash, line, 7);
        short_hash[7] = '\0';
        snprintf(out, out_size, "detached:%s", short_hash);
    }
    return 1;
}
