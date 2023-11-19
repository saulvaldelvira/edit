#ifndef LINE_H
#define LINE_H
#include "edit.h"

WString* current_line(void);
size_t current_line_length(void);
WString* line_at(int at);
size_t line_at_len(int at);
void line_insert(int at, const wchar_t *line, size_t len);
void line_append(const wchar_t *line, size_t len);
int line_cx_to_rx(WString *line, int cx);
void line_put_char(int c);
void line_move_up();
void line_move_down();
void line_delete_char_backwards(void);
void line_delete_char_forward(void);
void line_insert_newline(void);
void line_cut(bool whole);
void line_toggle_comment(void);
void line_strip_trailing_spaces(int cy);
void line_format(int cy);

#endif
