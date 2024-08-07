#ifndef UTIL_H
#define UTIL_H
#include "prelude.h"
#include <stdbool.h>
#include <stddef.h>

#include <log.h>

#define CLEANUP_GUARD do { \
        static bool _flag = false; \
        if (_flag) editor_log(LOG_WARN,"Cleanup function \"%s\" reached twice", __func__); \
        _flag = true; } while (0)

void NORETURN die(char *msg);
void NORETURN editor_end(void);
void* xmalloc(size_t nbytes);
void editor_scroll(void);
size_t wstrnlen(const wchar_t *wstr, size_t n);
int get_window_size(int *rows, int *cols);
char* editor_cwd(void);
void editor_update_render(void);
int get_character_width(wchar_t c, int accumulated_rx);
void free_wstr(void *e);
long get_time_millis(void);

#endif
