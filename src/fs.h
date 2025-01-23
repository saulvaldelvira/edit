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

char* get_tmp_filename(const wchar_t *filename);
char* get_data_directory(void);
char* get_config_directory(void);
void  mkdir_recursive(char *file_path, mode_t mode);
FILE* fopen_mkdir(char *fname, char *mode);
mode_t getumask(void);

#include <stdbool.h>

/* PLATFORM SPECIFIC: Refer to <platform_dir>/fs.c  */

bool is_dir(const char *filename);
bool file_exists(char *filename);
bool file_writable(char *filename);
bool is_link(char *filename);
int get_file_mode(char *fname);
bool change_mod(const char *fname, int mode);
int read_link(const char *lname, char *out, unsigned len);
int makedir(char *fname, int mode);

#ifdef _WIN32
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

#endif /* __FS_H__ */
