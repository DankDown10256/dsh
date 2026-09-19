#ifndef GIT_H
#define GIT_H
#include <stddef.h>

int is_git_repo(void);
int get_git_branch(char *out, size_t out_size);

#endif
