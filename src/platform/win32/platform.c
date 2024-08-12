#include "definitions.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fileapi.h>
#include <handleapi.h>
#include <minwindef.h>
#include <prelude.h>
#include <processenv.h>
#include <windows.h>
#include <wincon.h>
#include <shlwapi.h>
#include <winnls.h>
#include <winnt.h>
#include <io.h>
#include <winsock.h>

INLINE bool file_exists(char *filename) {
        return PathFileExists(filename) == 1;
}

INLINE bool file_writable(char *filename) {
        return _access(filename, 2) == 0
                || _access(filename, 6) == 0;
}

INLINE int read_link(const char *lname, char *out, unsigned len) {
        HANDLE h = CreateFile (
                        lname,
                        FILE_READ_ATTRIBUTES,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        0,
                        OPEN_EXISTING,
                        FILE_FLAG_BACKUP_SEMANTICS | FILE_ATTRIBUTE_REPARSE_POINT | FILE_FLAG_OPEN_REPARSE_POINT,
                        0);

        DeviceIoControl (
                        h,
                        FSCTL_GET_REPARSE_POINT,
                        NULL,
                        0,
                        out,
                        len,
                        NULL,
                        NULL);
        CloseHandle(h);
        return SUCCESS;
}

bool is_link(char *filename) {
        int attrs = GetFileAttributes(filename);
        return attrs & FILE_ATTRIBUTE_REPARSE_POINT;
}

int get_file_mode(char *fname) {
        struct _stat file_stat;
        _stat(fname, &file_stat);
        return file_stat.st_mode;
}

INLINE bool change_mod(const char *fname, int mode) {
        return _chmod(fname, mode) == 0;
}

INLINE int makedir(char *fname, int mode) {
        (void) mode;
        return CreateDirectoryA(fname,NULL);
}

int get_window_size(int *rows, int *cols) {
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO i;
        GetConsoleScreenBufferInfo(h, &i);
        *rows = i.dwSize.Y;
        *cols = i.dwSize.X;
        return SUCCESS;
}

static DWORD original_mode;

void enable_raw_mode(void) {
        HANDLE hh = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode;
        SetConsoleOutputCP(CP_UTF8);
        GetConsoleMode(hh, &mode);
        mode = 0;
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        mode |= ENABLE_PROCESSED_OUTPUT;
        mode |= ENABLE_WRAP_AT_EOL_OUTPUT;
        mode |= DISABLE_NEWLINE_AUTO_RETURN;
        SetConsoleMode(hh, mode);

        hh = GetStdHandle(STD_INPUT_HANDLE);
        SetConsoleOutputCP(CP_UTF8);
        GetConsoleMode(hh, &mode);
        mode = 0;
        mode &= ~ENABLE_ECHO_INPUT;
        mode &= ~ENABLE_LINE_INPUT;
        /* mode |= ENABLE_WINDOW_INPUT; */
        mode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
        SetConsoleMode(hh,mode);
}

void restore_termios(void) {
        /* HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE); */
        /* SetConsoleMode(h, original_mode); */
}

void switch_ctrl_c(bool allow) {
        (void)allow;
}

bool is_ctrl_c_pressed(void) {
        return false;
}

INLINE char* get_default_eol(void) {
        return "\r\n";
}
