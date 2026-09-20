#include <string.h>
#include "split_and.h"

int split_and(char **argv, char **left, char **right) {
    int i = 0, j = 0;
    int found = 0;

    while (argv[i] != NULL) {
        if (strcmp(argv[i], "&&") == 0) {
            left[i] = NULL;
            found = 1;
            i++;
            break;
        }
        left[i] = argv[i];
        i++;
    }
    if (!found) {
        return 0;
    }
    while (argv[i] != NULL) {
        right[j++] = argv[i++];
    }
    right[j] = NULL;
    return 1;
}
