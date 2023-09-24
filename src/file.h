#ifndef FILE_H
#define FILE_H

#include "edit.h"

int file_open(const wchar_t *filename);
void file_auto_save(void);
void file_reload(void);
int file_save(bool only_tmp, bool ask_filename);
char* get_tmp_filename(void);

#endif
