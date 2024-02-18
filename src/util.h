#ifndef UTIL_H
#define UTIL_H
#include <stddef.h>
#include <stdio.h>

#define die(msg)                                                     \
        do {                                                         \
                fprintf(stderr, "ERROR: %s. "                        \
                                "In %s, line %d (%s)\n",             \
                                msg, __FILE__, __LINE__, __func__);  \
                exit(1);                                             \
        }while (0)

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

#endif
