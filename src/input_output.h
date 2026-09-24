#ifndef INPUT_OUTPUT_H
#define INPUT_OUTPUT_H

typedef struct {
    char *out_file;
    int append;
    char *in_file;
} input_output_t;

int extract_redir(char **argv, input_output_t *redir);

#endif
