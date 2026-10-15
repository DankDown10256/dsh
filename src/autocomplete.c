#define _XOPEN_SOURCE 700
#define _POSIX_C_SOURCE 200809L
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <readline/readline.h>
#include "autocomplete.h"

#define MAX_COMPLETIONS 4096

static const char *const BUILTINS[] = {
    "cd", "exit", "help", "acpyvenv", "deactivate", "runwith", NULL
};

static char *completions[MAX_COMPLETIONS];
static int completion_count;

static int is_blank(char c) {
    return c == ' ' || c == '\t';
}

static int is_cmd_start(void) {
    for (int i = 0; i < (int)rl_point; i++) {
        if (!is_blank(rl_line_buffer[i])) {
            return 0;
        }
    }
    return 1;
}

static char *word_start(char *word, size_t word_size) {
    size_t start = rl_point;

    while (start > 0 && !is_blank(rl_line_buffer[start - 1])) {
        start--;
    }
    while (start > 0 && is_blank(rl_line_buffer[start - 1])) {
        start--;
    }

    size_t len = rl_point - start;
    if (len >= word_size) {
        len = word_size - 1;
    }
    memcpy(word, rl_line_buffer + start, len);
    word[len] = '\0';
    return word;
}

static void add_completion(const char *candidate) {
    if (completion_count >= MAX_COMPLETIONS) {
        return;
    }
    for (int i = 0; i < completion_count; i++) {
        if (strcmp(completions[i], candidate) == 0) {
            return;
        }
    }
    completions[completion_count] = strdup(candidate);
    if (completions[completion_count]) {
        completion_count++;
    }
}

static void reset_completions(void) {
    for (int i = 0; i < completion_count; i++) {
        free(completions[i]);
        completions[i] = NULL;
    }
    completion_count = 0;
}

static int is_exec(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode) && (st.st_mode & S_IXUSR);
}

static char *builtin_generator(const char *text, int state) {
    static size_t idx;
    size_t len = strlen(text);

    if (state == 0) {
        idx = 0;
    }
    while (BUILTINS[idx]) {
        const char *candidate = BUILTINS[idx++];
        if (strncmp(candidate, text, len) == 0) {
            return strdup(candidate);
        }
    }
    return NULL;
}

static char *path_generator(const char *text, int state) {
    static char *paths = NULL;
    static DIR *dir = NULL;
    static char path[PATH_MAX];
    size_t len = strlen(text);

    if (state == 0) {
        if (!paths) {
            const char *env = getenv("PATH");
            paths = strdup(env ? env : "");
        }
        char *save = NULL;
        char *tok = strtok_r(paths, ":", &save);
        while (tok) {
            if (tok[0] == '\0') {
                snprintf(path, sizeof(path), ".");
            } else {
                snprintf(path, sizeof(path), "%s", tok);
            }
            dir = opendir(path);
            if (dir) {
                break;
            }
            tok = strtok_r(NULL, ":", &save);
        }
        if (!dir) {
            free(paths);
            paths = NULL;
            return NULL;
        }
    }

    if (!dir) {
        return NULL;
    }

    struct dirent *entry;
    while ((entry = readdir(dir))) {
        char full[PATH_MAX];
        if (strncmp(entry->d_name, text, len) != 0) {
            continue;
        }
        if (text[0] != '.' && entry->d_name[0] == '.') {
            continue;
        }
        snprintf(full, sizeof(full), "%s/%s", path, entry->d_name);
        if (!is_exec(full)) {
            continue;
        }
        return strdup(entry->d_name);
    }

    closedir(dir);
    dir = NULL;
    if (paths) {
        free(paths);
        paths = NULL;
    }
    return NULL;
}

static char *dir_generator(const char *text, int state) {
    static DIR *dir = NULL;
    static char path[PATH_MAX];
    static size_t idx;
    size_t len = strlen(text);

    if (state == 0) {
        idx = 0;
        if (strchr(text, '/')) {
            char *slash = strrchr(text, '/');
            size_t dir_len = (size_t)(slash - text) + 1;
            if (dir_len >= sizeof(path)) {
                return NULL;
            }
            memcpy(path, text, dir_len);
            path[dir_len] = '\0';
            text = slash + 1;
            len = strlen(text);
        } else {
            snprintf(path, sizeof(path), ".");
        }
        dir = opendir(path);
        if (!dir) {
            return NULL;
        }
    }

    if (!dir) {
        return NULL;
    }

    struct dirent *entry;
    while ((entry = readdir(dir))) {
        char full[PATH_MAX];
        if (strncmp(entry->d_name, text, len) != 0) {
            continue;
        }
        if (text[0] != '.' && entry->d_name[0] == '.') {
            continue;
        }
        snprintf(full, sizeof(full), "%s%s", path, entry->d_name);
        add_completion(full);
    }

    closedir(dir);
    dir = NULL;

    if (completion_count == 0) {
        return NULL;
    }
    return strdup(completions[idx++]);
}

static char **dsh_completion(const char *text, int start, int end) {
    char word[256];
    char *word_begin = word_start(word, sizeof(word));

    rl_completion_append_character = ' ';
    rl_completion_suppress_append = 0;

    if (is_cmd_start()) {
        char **matches = rl_completion_matches(text, builtin_generator);
        if (matches) {
            return matches;
        }
        return rl_completion_matches(text, path_generator);
    }

    char *slash = strrchr(word_begin, '/');
    char adjusted[512];
    char new_line[2048];
    char *save_line = rl_line_buffer;
    int save_point = rl_point;

    if (slash) {
        size_t dir_len = (size_t)(slash - word_begin) + 1;
        memcpy(adjusted, word_begin, dir_len);
        adjusted[dir_len] = '\0';
    } else {
        snprintf(adjusted, sizeof(adjusted), "./%s", word_begin);
    }

    strcpy(new_line, rl_line_buffer);
    new_line[start] = '\0';
    strncat(new_line, adjusted, sizeof(new_line) - strlen(new_line) - 1);
    strncat(new_line, rl_line_buffer + end, sizeof(new_line) - strlen(new_line) - 1);
    rl_line_buffer = new_line;
    rl_point = start;

    reset_completions();
    char **matches = rl_completion_matches(adjusted, dir_generator);
    rl_line_buffer = save_line;
    rl_point = save_point;
    return matches;
}

void autocomplete_setup(void) {
    rl_attempted_completion_function = dsh_completion;
}
