#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdbool.h>

bool file_exists(char *filename);
bool file_writable(char *filename);

bool is_link(char *filename);
int get_file_mode(char *fname);
bool change_mod(char *fname, int mode);

int read_link(const char *lname, char *out, unsigned len);

#endif
