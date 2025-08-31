#ifndef UTIL_H
#define UTIL_H
#include "prelude.h"
#include <stdbool.h>
#include <stddef.h>

#include <log.h>

void* xmalloc(size_t nbytes);
void editor_scroll(void);
size_t wstrnlen(const wchar_t *wstr, size_t n);
size_t wstrlen(const wchar_t *wstr);
wchar_t* wstrdup(const wchar_t *wstr);
char* editor_cwd(void);
int get_character_width(wchar_t c, int accumulated_rx);
void free_wstr(void *e);
long get_time_millis(void);
int get_cursor_position(int *row, int *col);

void save_history_to_file(char *sub_path, vector_t *entries);
vector_t* load_history_from_file(char *sub_path);

#endif
