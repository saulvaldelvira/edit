#ifndef LINE_H
#define LINE_H
#include "prelude.h"

string_t* current_line(void);
size_t current_line_length(void);
string_t* line_at(int at);
size_t line_at_len(int at);
wchar_t line_curr_char(void);
void line_insert(int at, const char *line, size_t len);
void line_append(const char *line, size_t len);
int line_cx_to_rx(string_t *line, int cx);
void line_put_char(int c);
void line_move_up(void);
void line_move_down(void);
void line_delete_char_backwards(void);
void line_delete_char_forward(void);
void line_delete_word_forward(void);
void line_delete_word_backwards(void);
void line_insert_newline(void);
void line_cut(bool whole);
void line_toggle_comment(void);
void line_strip_trailing_spaces(int cy);
void line_format(int cy);

#endif
