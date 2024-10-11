#include "line.h"
#include <console/io.h>
#include "prelude.h"
#include "util.h"
#include <console/cursor.h>
#include "buffer/mode.h"
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <assert.h>

string_t* line_at(int at){
	string_t *line = NULL;
	if (at < buffers.curr->num_lines)
		vector_at(buffers.curr->lines, at, &line);
	return line;
}

size_t line_at_len(int at){
	string_t *line = line_at(at);
	return line ? str_length_utf8(line) : 0UL;
}

INLINE
string_t* current_line(void){
	return line_at(buffers.curr->cy);
}

INLINE
size_t current_line_length(void){
	return line_at_len(buffers.curr->cy);
}

wchar_t line_curr_char(void){
	string_t *line = current_line();
	if (!line) return '\0';
	return str_get_at(line, buffers.curr->cx);
}

void line_insert(int at, const char *str, size_t len){
	string_t *line = str_from_cstr(str, len);
	vector_insert_at(buffers.curr->lines, at, &line);
	buffers.curr->num_lines++;
}

void line_append(const char *str, size_t len){
	string_t *line = str_from_cstr(str, len);
	vector_append(buffers.curr->lines, &line);
	buffers.curr->num_lines++;
}

int line_cx_to_rx(string_t *line, int cx){
	int rx = 0;
	for (int i = 0; i < cx; i++){
		wchar_t c = str_get_at(line, i);
		rx += get_character_width(c, rx);
	}
	return rx;
}

void line_move_up(void){
	if (buffers.curr->cy == 0) return;
	vector_swap(buffers.curr->lines, buffers.curr->cy, buffers.curr->cy - 1);
	buffers.curr->cy--;
	buffers.curr->dirty++;
}

void line_move_down(void){
	if (buffers.curr->cy == buffers.curr->num_lines - 1) return;
	vector_swap(buffers.curr->lines, buffers.curr->cy, buffers.curr->cy + 1);
	buffers.curr->cy++;
	buffers.curr->dirty++;
}

void line_put_char(int c){
	if (buffers.curr->cy == buffers.curr->num_lines)
		line_insert(buffers.curr->num_lines, "", 0);
	string_t *line;
	vector_at(buffers.curr->lines, buffers.curr->cy, &line);
	int n = 1;
	if (c == L'\t' && buffers.curr->conf.substitute_tab_with_space){
		n = get_character_width('\t', buffers.curr->cx);
		for (int i = 0; i < n; i++)
			str_insert(line, ' ', buffers.curr->cx);
	}else {
		str_insert(line, c, buffers.curr->cx);
	}
	buffers.curr->cx += n;
	buffers.curr->dirty += n;
}

void line_delete_char_forward(void){
	if (buffers.curr->cy == buffers.curr->num_lines)
		return;
	if (buffers.curr->cx == 0 && current_line_length() == 0){
		vector_remove_at(buffers.curr->lines, buffers.curr->cy);
		buffers.curr->num_lines--;
	}else{
		cursor_move(ARROW_RIGHT);
		line_delete_char_backwards();
	}
}

void line_delete_char_backwards(void){
	if (buffers.curr->cy == buffers.curr->num_lines){
		cursor_move(ARROW_LEFT);
		return;
	}
	string_t *line;
	vector_at(buffers.curr->lines, buffers.curr->cy, &line);
	if (buffers.curr->cx == 0){
		if (buffers.curr->cy == 0)
			return;
		string_t *up;
		vector_at(buffers.curr->lines, buffers.curr->cy - 1, &up);
		size_t new_x = str_length_utf8(up);
		str_concat_str(up, line);
		vector_remove_at(buffers.curr->lines, buffers.curr->cy);
		buffers.curr->num_lines--;
		cursor_move(ARROW_LEFT);
		buffers.curr->cx = new_x;
	}
	else if (buffers.curr->cx > 0 && (size_t)(buffers.curr->cx - 1) < str_length_utf8(line)){
		str_remove_at(line, buffers.curr->cx - 1);
		cursor_move(ARROW_LEFT);
	}
	buffers.curr->dirty++;
}

void line_delete_word_forward(void){
	if ((size_t)buffers.curr->cx == current_line_length()){
		line_delete_char_forward();
		return;
	}
	int x = buffers.curr->cx;
	cursor_jump_word(ARROW_RIGHT);
	int diff = buffers.curr->cx - x;
	while (diff-- > 0)
		line_delete_char_backwards();
}

void line_delete_word_backwards(void){
	if (buffers.curr->cx == 0){
		line_delete_char_backwards();
		return;
	}
	int x = buffers.curr->cx;
	cursor_jump_word(ARROW_LEFT);
	int diff = x - buffers.curr->cx;
	while (diff-- > 0)
		line_delete_char_forward();
}

void line_insert_newline(void){
	if (buffers.curr->cx == 0){
		line_insert(buffers.curr->cy, "", 0);
	}else{
		string_t *current;
		vector_at(buffers.curr->lines, buffers.curr->cy, &current);
		char *split = str_substring(current, buffers.curr->cx, str_length_utf8(current));
		size_t split_len = str_length_utf8(current) - buffers.curr->cx;
		str_remove_range(current, buffers.curr->cx, str_length_utf8(current));
		line_insert(buffers.curr->cy + 1, split, split_len);
		free(split);
	}
	buffers.curr->cy++;
	buffers.curr->cx = 0;
	buffers.curr->dirty++;
}

void line_cut(bool whole){
	if (buffers.curr->cy >= buffers.curr->num_lines)
		return;
	if (whole){
		vector_remove_at(buffers.curr->lines, buffers.curr->cy);
		buffers.curr->num_lines--;
		cursor_adjust();
	}else {
		string_t *line = current_line();
		str_remove_range(line, buffers.curr->cx, -1);
	}
	buffers.curr->dirty++;
}

void line_toggle_comment(void){
	int mode = mode_get_current();
	if (mode == NO_MODE) return;
	const char *comment_start = mode_comments[mode][0];
	const char *comment_end = mode_comments[mode][1];

	string_t *line = current_line();
	int match = str_find_substring(line, comment_start, 0);
	// TODO: Replace only the outermost comment
	if (match == 0)
		str_replace(line, comment_start, "");
	else
		str_insert_cstr(line, comment_start, -1, 0);

	if (comment_end == NULL) return;
	if (match != 0){
		str_concat_cstr(line, comment_end, -1);
		return;
	}
	match = str_find_substring(line, comment_end, 0);
	if (match >= 0)
		str_replace(line, comment_end, "");
}

void line_strip_trailing_spaces(int cy){
	if (cy >= buffers.curr->num_lines)
		return;
	assert(cy >= 0);
	string_t *line;
	vector_at(buffers.curr->lines, cy, &line);
	size_t len = str_length_utf8(line);
	size_t last_char_index = 0;
	for (size_t i = 0; i < len; i++){
		wchar_t c = str_get_at(line, i);
		if (c != L' ' && c != L'\t')
			last_char_index = i;
	}
	if (last_char_index < len){
		size_t removed;
		wchar_t c = str_get_at(line, 0);
		if (last_char_index == 0 && (c == L' ' || c == L'\t')){
			str_remove_range(line, 0, len);
			removed = len;
		}else{
			str_remove_range(line, last_char_index + 1, len);
			removed = len - last_char_index - 1;
		}
		if (cy == buffers.curr->cy){
			if ((size_t)buffers.curr->cx >= removed){
				buffers.curr->cx -= removed;
			}else{
				buffers.curr->col_offset -= removed;
				if (buffers.curr->col_offset < 0){
					buffers.curr->cx += buffers.curr->col_offset;
					buffers.curr->col_offset = 0;
				}
			}
		}
	}
}

void line_format(int cy){
	line_strip_trailing_spaces(cy);
	char tab_buffer[80];
	snprintf(tab_buffer, 80, "%*c", buffers.curr->conf.tab_size, ' ');
	string_t *line = line_at(cy);
	if (buffers.curr->conf.substitute_tab_with_space){
		int n = str_replace(line, "\t", tab_buffer);
                buffers.curr->cx += n * (buffers.curr->conf.tab_size + 1);
	}else{
		int n = str_replace(line, tab_buffer, "\t");
                buffers.curr->cx -= n * (buffers.curr->conf.tab_size - 1);
	}
}
