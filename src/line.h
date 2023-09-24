#ifndef LINE_H
#define LINE_H
#include "edit.h"

WString* current_line(void);
size_t current_line_length(void);
void line_insert(int at, const wchar_t *line, size_t len);
int line_cx_to_rx(WString *line, int cx);
void line_put_char(int c);
void line_delete_char(void);
void line_insert_newline(void);
void line_cut(void);
void line_toogle_comment(void);
void line_strip_trailing_spaces(int cy);

#endif
