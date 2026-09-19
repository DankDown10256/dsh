#ifndef VENV_H
#define VENV_H

#include <stddef.h>

int is_python_venv(char *out_path, size_t out_size);
int activate_python_venv(const char *venv_path);
int deactivate_python_venv(void);

#endif
