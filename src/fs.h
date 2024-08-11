#ifndef __FS_H__
#define __FS_H__

#include <stdio.h>
#include <definitions.h>
#include <sys/types.h>

char* get_data_directory(void);
char* get_config_directory(void);
void  mkdir_recursive(char *file_path, mode_t mode);
FILE* fopen_mkdir(char *fname, char *mode);
mode_t getumask(void);

#endif /* __FS_H__ */
