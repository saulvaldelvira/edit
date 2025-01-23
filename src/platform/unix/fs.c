#include <prelude.h>
#include <sys/stat.h>
#include <unistd.h>
#include <termios.h>
#include <util.h>
#include <sys/ioctl.h>

bool is_dir(const char *filename) {
        struct stat statbuf;
        if (stat(filename, &statbuf) != 0)
                return 0;
        return S_ISDIR(statbuf.st_mode);
}

bool file_exists(char *filename) {
        return access(filename, F_OK) == 0;
}

bool file_writable(char *filename) {
        return access(filename, W_OK) == 0;
}

int read_link(const char *lname, char *out, unsigned len) {
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

bool change_mod(const char *fname, int mode) {
        return chmod(fname, mode) == 0;
}

int makedir(char *fname, int mode) {
        return mkdir(fname,mode);
}
