#include <prelude.h>
#include <platform.h>
#include <sys/stat.h>
#include <unistd.h>

INLINE bool file_exists(char *filename) {
        return access(filename, F_OK) == 0;
}

INLINE bool file_writable(char *filename) {
        return access(filename, W_OK) == 0;
}

INLINE int read_link(const char *lname, char *out, unsigned len) {
        return readlink(lname, out, len) > 0;
}

bool is_link(char *filename) {
        struct stat file_stat = {0};
	lstat(filename, &file_stat);
	return S_ISLNK(file_stat.st_mode);
}

int get_file_mode(char *fname) {
        struct stat file_stat;
        stat(fname, &file_stat);
        return file_stat.st_mode;
}

INLINE bool change_mod(char *fname, int mode) {
        return chmod(fname, mode) == 0;
}
