#ifndef FILE_H
#define FILE_H

#include <stdbool.h>

int file_open(const char *filename);
void file_auto_save(void);
void file_reload(void);
int file_save(bool only_tmp, bool ask_filename);
char* get_tmp_filename(void);

void init_file(void);

#endif
