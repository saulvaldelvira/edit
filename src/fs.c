#include <prelude.h>
#include "limits.h"
#include "platform.h"
#include <fs.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#ifdef _WIN32
#define SEP '\\'
#else
#define SEP '/'
#endif

void  mkdir_recursive(char *file_path, mode_t mode) {
        if (!strchr(file_path, SEP))
                return;
        char dir_path[PATH_MAX];
        strncpy(dir_path, file_path, PATH_MAX - 1);
        dir_path[PATH_MAX - 1] = '\0';
        char *next_sep = strchr(dir_path, SEP);
        while (next_sep != NULL) {
                *next_sep = '\0';
                makedir(dir_path, mode);
                *next_sep = SEP;
                next_sep = strchr(next_sep + 1, SEP);
        }
}

FILE* fopen_mkdir(char *fname, char *mode) {
        char buf[PATH_MAX];
        strncpy(buf, fname, PATH_MAX);
        buf[PATH_MAX - 1] = '\0';
        char *last = strrchr(buf, SEP);
        if (last) {
                *last = '\0';
                mkdir_recursive(buf, getumask());
        }
        return fopen(fname, mode);
}

#define DIR "edit"

char* get_config_directory(void) {
        static char buf[NAME_MAX];

        char *dir = getenv("XDG_CONFIG_HOME");
        if (!dir) {
                dir = getenv("HOME");
                if (!dir) return NULL;
                sprintf(buf, "%s/.config/" DIR, dir);
        } else {
                sprintf(buf, "%s/" DIR, dir);
        }
        return buf;
}

char* get_data_directory(void) {
        static char buf[NAME_MAX];

        char *dir = getenv("XDG_DATA_HOME");
        if (!dir) {
                dir = getenv("HOME");
                if (!dir) return NULL;
                sprintf(buf, "%s/.local/share/" DIR, dir);
        } else {
                sprintf(buf, "%s/" DIR, dir);
        }
        mkdir_recursive(buf, getumask());
        return buf;
}

INLINE mode_t getumask(void) {
        mode_t mask = umask( 0 );
        umask(mask);
        return mask;
}
