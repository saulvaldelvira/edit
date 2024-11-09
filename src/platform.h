#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdbool.h>

bool file_exists(char *filename);
bool file_writable(char *filename);

bool is_link(char *filename);
int get_file_mode(char *fname);
bool change_mod(const char *fname, int mode);

int read_link(const char *lname, char *out, unsigned len);

int makedir(char *fname, int mode);

void switch_ctrl_c(bool allow);
bool is_ctrl_c_pressed(void);

int get_window_size(int *rows, int *cols);

void enable_raw_mode(void);
void restore_termios(void);

char* get_default_eol(void);

#ifdef _WIN32
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

#endif
