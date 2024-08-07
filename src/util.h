#ifndef UTIL_H
#define UTIL_H
#include <stdbool.h>
#include <stddef.h>

#include <log.h>

#define CLEANUP_GUARD do { \
        static bool _flag = false; \
        if (_flag) editor_log("Cleanup function \"%s\" reached twice\n", __func__); \
        _flag = true; } while (0)

void die(char *msg);
void* xmalloc(size_t nbytes);
void editor_end(void);
void editor_scroll(void);
size_t wstrnlen(const wchar_t *wstr, size_t n);
int get_window_size(int *rows, int *cols);
char* editor_cwd(void);
void editor_update_render(void);
int get_character_width(wchar_t c, int accumulated_rx);
void free_wstr(void *e);
long get_time_millis(void);
bool file_exists(char *filename);

#endif
