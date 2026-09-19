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
    char *tok = strtok(line, " \t\n");
    while (tok && argc < MAX_ARGS - 1) {
        argv[argc ++] = tok;
        tok = strtok(NULL, " \t\n");
    }
    argv[argc] = NULL;
    return argc;
}

static int is_git_repo(void) {
    struct stat st;
    return stat(".git", &st) == 0 && S_ISDIR(st.st_mode);
}

static int get_git_branch(char *out, size_t out_size) {
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

static int run_builtin(char **argv) {
    if (strcmp(argv[0], "exit") == 0) {
        exit(0);
    }
    if (strcmp(argv[0], "cd") == 0) {
        const char *target = argv[1] ? argv[1] : getenv("HOME");
        if (chdir(target) != 0) perror("cd");
        return 1;
    }
    return 0;
}

static void run_external(char **argv) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }
    if (pid == 0) {
        execvp(argv[0], argv);
        fprintf(stderr, "%s: command not found\n", argv[0]);
        _exit(127);
    }
    int status;
    waitpid(pid, &status, 0);
}

int main(void) {
    char line[MAX_LINE];
    char *argv[MAX_ARGS];
    char branch[128];

    struct passwd *pw = getpwuid(getuid());
    const char *user = pw ? pw->pw_name : "?";

    while (1) {
        if (is_git_repo() && get_git_branch(branch, sizeof(branch))) {
            printf("[%s %s@dsh] ", branch, user);
        } else {
            printf("[%s@dsh] ", user);
        }
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }

        int argc = parse_line(line, argv);
        if (argc == 0) continue;

        if (run_builtin(argv)) continue;

        run_external(argv);
    }

    return 0;
}
