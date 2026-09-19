#include <stddef.h>
#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/stat.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <limits.h>
#include <time.h>
#include <signal.h>
#include "src/py_venv.h"
#include "src/git.h"

#define SLOW_CMD_THRESHOLD_SEC 5
#define MAX_ARGS 64
#define MAX_LINE 1024
#define COLOR_RESET   "\033[0m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_RED     "\033[31m"
#define COLOR_CYAN    "\033[36m"

static int parse_line(char *line, char **argv) {
    int argc = 0;
    char *p = line;

    while (*p) {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0') break;

        if (*p == '"') {
            p++;
            argv[argc++] = p;
            while (*p && *p != '"') p++;
            if (*p == '"') {
                *p = '\0';
                p++;
            }
        } else {
            argv[argc++] = p;
            while (*p && *p != ' ' && *p != '\t') p++;
            if (*p) {
                *p = '\0';
                p++;
            }
        }

        if (argc >= MAX_ARGS - 1) break;
    }

    argv[argc] = NULL;
    return argc;
}

static void ensure_default_path(void) {
    char paths[2048];
    struct passwd *pw = getpwuid(getuid());
    const char *user = pw ? pw->pw_name : "?";
    snprintf(paths, sizeof(paths), "/usr/local/bin:/usr/bin:/home/%s/.local/bin:/bin", user);
    const char *default_paths = paths;
    const char *current_path = getenv("PATH");

    if (!current_path || strlen(current_path) == 0) {
        setenv("PATH", default_paths, 1);
        return;
    }

    char new_path[4096];
    snprintf(new_path, sizeof(new_path), "%s:%s", current_path, default_paths);
    setenv("PATH", new_path, 1);
}

static int run_builtin(char **argv) {
    if (strcmp(argv[0], "exit") == 0) {
        exit(0);
    }
    if (strcmp(argv[0], "cd") == 0) {
        const char *target = argv[1] ? argv[1] : getenv("HOME");
        if (chdir(target) != 0) perror("cd");
        return 1;
    }
    if (strcmp(argv[0], "acpyvenv") == 0) {
        char venv_path[64];
        if (is_python_venv(venv_path, sizeof(venv_path))) {
            activate_python_venv(venv_path);
        } else {
            printf("Sorry there isn't python env in this directory\n");
        }
        return 1;
    }
    if (strcmp(argv[0], "deactivate") == 0) {
        deactivate_python_venv();
        return 1;
    }
    if (strcmp(argv[0], "help") == 0) {
        printf("Help Menu\n");
        printf("Commands:\n");
        printf("help: show this menu\n");
        printf("ls: list directories\n");
        printf("cd: move to a given directory path\n");
        printf("acpyvenv: detect and activate a python venv\n");
    }
    return 0;
}

static void run_external(char **argv) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    char *new_argv[MAX_ARGS];
    if (strcmp(argv[0], "ls") == 0) {
        new_argv[0] = "ls";
        new_argv[1] = "-al";
        new_argv[2] = "--color=auto";
        int i = 1, j = 2;
        while (argv[i] != NULL) {
            new_argv[j++] = argv[i++];
        }
        new_argv[j] = NULL;
        argv = new_argv;
    }
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }
    if (pid == 0) {
        signal(SIGINT, SIG_DFL);
        execvp(argv[0], argv);
        fprintf(stderr, "%s: command not found\n", argv[0]);
        _exit(127);
    }
    int status;
    waitpid(pid, &status, 0);
    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    if (elapsed >= SLOW_CMD_THRESHOLD_SEC) {
        char notif_cmd[256];
        snprintf(notif_cmd, sizeof(notif_cmd), "notify-send 'dsh' 'Command %s is finished and took %1.fs'", argv[0], elapsed);
        system(notif_cmd);
    }
}

int main(void) {
    signal(SIGINT, SIG_IGN);
    ensure_default_path();
    char line[MAX_LINE];
    char *argv[MAX_ARGS];
    char branch[128];
    static char histfile[PATH_MAX];
    const char *home = getenv("HOME");
    if (home) {
        snprintf(histfile, sizeof(histfile), "%s/.dsh_history", home);
        read_history(histfile);
    }
    struct passwd *pw = getpwuid(getuid());
    const char *user = pw ? pw->pw_name : "?";

    while (1) {
        char prompt[512];
        char venv_path[64];
        char cwd[PATH_MAX];
        char display_cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            strcpy(cwd, "?");
        }
        const char *home = getenv("HOME");
        if (home && strncmp(cwd, home, strlen(home)) == 0) {
            snprintf(display_cwd, sizeof(display_cwd), "~%s", cwd + strlen(home));
        } else {
            snprintf(display_cwd, sizeof(display_cwd), "%s", cwd);
        }
        int has_git = is_git_repo() && get_git_branch(branch, sizeof(branch));
        int has_venv = is_python_venv(venv_path, sizeof(venv_path));
        if (has_git) {
            snprintf(prompt, sizeof(prompt), "[%s %s@dsh in %s] ", branch, user, display_cwd);
        } else {
            snprintf(prompt, sizeof(prompt), "[%s@dsh in %s] ", user, display_cwd);
        }

        char *input = readline(prompt);
        if (!input) {
            break;
        }

        strncpy(line, input, MAX_LINE - 1);
        line[MAX_LINE - 1] = '\0';
        add_history(line);
        free(input);

        int argc = parse_line(line, argv);
        if (argc == 0) continue;

        if (run_builtin(argv)) continue;

        run_external(argv);
    }

    return 0;
}
