#include "line.h"
#include <console/io.h>
#include "prelude.h"
#include "util.h"
#include <console/cursor.h>
#include "buffer/mode.h"
#include <stdlib.h>
#include <wchar.h>
#include <assert.h>

wstring_t* line_at(int at){
	wstring_t *line = NULL;
	if (at < buffers.curr->num_lines)
		vector_at(buffers.curr->lines, at, &line);
	return line;
}

size_t line_at_len(int at){
	wstring_t *line = line_at(at);
	return line ? wstr_length(line) : 0UL;
}

INLINE
wstring_t* current_line(void){
	return line_at(buffers.curr->cy);
}

INLINE
size_t current_line_length(void){
	return line_at_len(buffers.curr->cy);
}

wchar_t line_curr_char(void){
	wstring_t *line = current_line();
	if (!line) return '\0';
	return wstr_get_at(line, buffers.curr->cx);
}

void line_insert(int at, const wchar_t *str, size_t len){
	wstring_t *line = wstr_from_cwstr(str, len);
	vector_insert_at(buffers.curr->lines, at, &line);
	buffers.curr->num_lines++;
}

void line_append(const wchar_t *str, size_t len){
	wstring_t *line = wstr_from_cwstr(str, len);
	vector_append(buffers.curr->lines, &line);
	buffers.curr->num_lines++;
}

int line_cx_to_rx(wstring_t *line, int cx){
	int rx = 0;
	for (int i = 0; i < cx; i++){
		wchar_t c = wstr_get_at(line, i);
		rx += get_character_width(c, rx);
	}
	return rx;
}

static void line_move_up(void){
	if (buffers.curr->cy == 0) return;
	vector_swap(buffers.curr->lines, buffers.curr->cy, buffers.curr->cy - 1);
	buffers.curr->cy--;
	buffers.curr->dirty++;
}

static void line_move_down(void){
	if (buffers.curr->cy == buffers.curr->num_lines - 1) return;
	vector_swap(buffers.curr->lines, buffers.curr->cy, buffers.curr->cy + 1);
	buffers.curr->cy++;
	buffers.curr->dirty++;
}

int line_move(cursor_direction_t dir) {
        switch (dir) {
        case CURSOR_DIRECTION_UP:
                line_move_up(); break;
        case CURSOR_DIRECTION_DOWN:
                line_move_down(); break;
        default:
                return -1;
        }
        return 1;
}

static void __insert_newline(void){
	if (buffers.curr->cx == 0){
		line_insert(buffers.curr->cy, L"", 0);
	}else{
		wstring_t *current;
		vector_at(buffers.curr->lines, buffers.curr->cy, &current);
		wchar_t *split = wstr_substring(current, buffers.curr->cx, wstr_length(current));
		size_t split_len = wstr_length(current) - buffers.curr->cx;
		wstr_remove_range(current, buffers.curr->cx, wstr_length(current));
		line_insert(buffers.curr->cy + 1, split, split_len);
		free(split);
	}
	buffers.curr->cy++;
	buffers.curr->cx = 0;
	buffers.curr->dirty++;
}


void line_put_char(int c){
        if (c == '\r' || c == '\n') {
                __insert_newline();
                return;
        }
	if (buffers.curr->cy == buffers.curr->num_lines)
		line_insert(buffers.curr->num_lines, L"", 0);
	wstring_t *line;
	vector_at(buffers.curr->lines, buffers.curr->cy, &line);
	int n = 1;
	if (c == L'\t' && buffers.curr->conf.substitute_tab_with_space){
		n = get_character_width(L'\t', buffers.curr->cx);
		for (int i = 0; i < n; i++)
			wstr_insert(line, ' ', buffers.curr->cx);
	}else {
		wstr_insert(line, c, buffers.curr->cx);
	}
	buffers.curr->cx += n;
	buffers.curr->dirty += n;
}

static void line_delete_char_backwards(void){
	if (buffers.curr->cy == buffers.curr->num_lines){
		cursor_move(CURSOR_DIRECTION_LEFT);
		return;
	}
	wstring_t *line;
	vector_at(buffers.curr->lines, buffers.curr->cy, &line);
	if (buffers.curr->cx == 0){
		if (buffers.curr->cy == 0)
			return;
		wstring_t *up;
		vector_at(buffers.curr->lines, buffers.curr->cy - 1, &up);
		size_t new_x = wstr_length(up);
		wstr_concat_wstr(up, line);
		vector_remove_at(buffers.curr->lines, buffers.curr->cy);
		buffers.curr->num_lines--;
		cursor_move(CURSOR_DIRECTION_LEFT);
		buffers.curr->cx = new_x;
	}
	else if (buffers.curr->cx > 0 && (size_t)(buffers.curr->cx - 1) < wstr_length(line)){
		wstr_remove_at(line, buffers.curr->cx - 1);
		cursor_move(CURSOR_DIRECTION_LEFT);
	}
	buffers.curr->dirty++;
}

static void line_delete_char_forward(void){
	if (buffers.curr->cy == buffers.curr->num_lines)
		return;
	if (buffers.curr->cx == 0 && current_line_length() == 0){
		vector_remove_at(buffers.curr->lines, buffers.curr->cy);
		buffers.curr->num_lines--;
	}else{
		cursor_move(CURSOR_DIRECTION_RIGHT);
		line_delete_char_backwards();
	}
}

int line_delete_char(cursor_direction_t dir) {
        switch (dir) {
        case CURSOR_DIRECTION_LEFT:
                line_delete_char_backwards(); break;
        case CURSOR_DIRECTION_RIGHT:
                line_delete_char_forward(); break;
        default:
                return -1;
        }
        return 1;
}

static void line_delete_word_forward(void){
	if ((size_t)buffers.curr->cx == current_line_length()){
		line_delete_char_forward();
		return;
	}
	int x = buffers.curr->cx;
	cursor_jump_word(CURSOR_DIRECTION_RIGHT);
	int diff = buffers.curr->cx - x;
	while (diff-- > 0)
		line_delete_char_backwards();
}

static void line_delete_word_backwards(void){
	if (buffers.curr->cx == 0){
		line_delete_char_backwards();
		return;
	}
	int x = buffers.curr->cx;
	cursor_jump_word(CURSOR_DIRECTION_LEFT);
	int diff = x - buffers.curr->cx;
	while (diff-- > 0)
		line_delete_char_forward();
}

int line_delete_word(cursor_direction_t dir) {
        switch (dir) {
        case CURSOR_DIRECTION_RIGHT:
                line_delete_word_forward(); break;
        case CURSOR_DIRECTION_LEFT:
                line_delete_word_backwards(); break;
        default:
                return -1;
        }
        return 1;
}

void line_cut(bool whole){
	if (buffers.curr->cy >= buffers.curr->num_lines)
		return;
	if (whole){
		vector_remove_at(buffers.curr->lines, buffers.curr->cy);
		buffers.curr->num_lines--;
		cursor_adjust();
	}else {
		wstring_t *line = current_line();
		wstr_remove_range(line, buffers.curr->cx, -1);
	}
	buffers.curr->dirty++;
}

void line_toggle_comment(void){
	int mode = mode_get_current();
	if (mode == NO_MODE) return;
	const wchar_t *comment_start = mode_comments[mode][0];
	const wchar_t *comment_end = mode_comments[mode][1];

	wstring_t *line = current_line();
	int match = wstr_find_substring(line, comment_start, 0);
	// TODO: Replace only the outermost comment
	if (match == 0)
		wstr_replace(line, comment_start, L"");
	else
		wstr_insert_cwstr(line, comment_start, -1, 0);

	if (comment_end == NULL) return;
	if (match != 0){
		wstr_concat_cwstr(line, comment_end, -1);
		return;
	}
	match = wstr_find_substring(line, comment_end, 0);
	if (match >= 0)
		wstr_replace(line, comment_end, L"");
}

void line_strip_trailing_spaces(int cy){
	if (cy >= buffers.curr->num_lines)
		return;
	assert(cy >= 0);
	wstring_t *line;
	vector_at(buffers.curr->lines, cy, &line);
	size_t len = wstr_length(line);
	size_t last_char_index = 0;
	for (size_t i = 0; i < len; i++){
		wchar_t c = wstr_get_at(line, i);
		if (c != L' ' && c != L'\t')
			last_char_index = i;
	}
	if (last_char_index < len){
		size_t removed;
		wchar_t c = wstr_get_at(line, 0);
		if (last_char_index == 0 && (c == L' ' || c == L'\t')){
			wstr_remove_range(line, 0, len);
			removed = len;
		}else{
			wstr_remove_range(line, last_char_index + 1, len);
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
	wchar_t tab_buffer[80];
	swprintf(tab_buffer, 80, L"%*c", buffers.curr->conf.tab_size, ' ');
	wstring_t *line = line_at(cy);
	if (buffers.curr->conf.substitute_tab_with_space){
		int n = wstr_replace(line, L"\t", tab_buffer);
                buffers.curr->cx += n * (buffers.curr->conf.tab_size + 1);
	}else{
		int n = wstr_replace(line, tab_buffer, L"\t");
                buffers.curr->cx -= n * (buffers.curr->conf.tab_size - 1);
	}
}

INLINE
void line_format_current(void) {
        line_format(current_line_row());
}

INLINE
int current_line_row(void) {
        return buffers.curr->cy;
}
