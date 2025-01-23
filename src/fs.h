#ifndef __FS_H__
#define __FS_H__

#include <stdio.h>
#include <definitions.h>
#include <sys/types.h>
#include <stdbool.h>
#include <wchar.h>

int file_open(const wchar_t *filename);
void file_auto_save(void);
void file_reload(void);
int file_save(bool only_tmp, bool ask_filename);

char* get_tmp_filename(void);
char* get_data_directory(void);
char* get_config_directory(void);
void  mkdir_recursive(char *file_path, mode_t mode);
FILE* fopen_mkdir(char *fname, char *mode);
mode_t getumask(void);

#endif /* __FS_H__ */
