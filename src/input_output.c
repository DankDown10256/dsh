#include <fcntl.h>
#include "input_output.h"
#include <string.h>

int extract_redir(char **argv, input_output_t *redir) {
    redir->out_file = NULL;
    redir->in_file = NULL;
    redir->append = 0;

    int write_idx = 0;
    for (int i = 0; argv[i] != NULL; i++) {
        if (strcmp(argv[i], ">") == 0 && argv[i + 1]) {
            redir->out_file = argv[i + 1];
            redir->append = 0;
            i++;
        } else if (strcmp(argv[i], ">>") == 0 && argv[i + 1]) {
            redir->out_file = argv[i + 1];
            redir->append = 1;
            i++;
        } else if (strcmp(argv[i], "<") == 0 && argv[i + 1]) {
            redir->in_file = argv[i + 1];
            i++;
        } else {
            argv[write_idx++] = argv[i];
        }
    }
    argv[write_idx] = NULL;
    return 1;
}
