#include "line.h"
#include "input.h"
#include "util.h"
#include "cursor.h"
#include "mode.h"
#include <stdlib.h>
#include <wchar.h>
#include <assert.h>

WString* line_at(int at){
	WString *line = NULL;
	if (at < conf.num_lines)
		vector_at(conf.lines, at, &line);
	return line;
}

size_t line_at_len(int at){
	WString *line = line_at(at);
	return line ? wstr_length(line) : 0UL;
}

WString* current_line(void){
	return line_at(conf.cy);
}

size_t current_line_length(void){
	return line_at_len(conf.cy);
}

wchar_t line_curr_char(void){
	WString *line = current_line();
	if (!line) return '\0';
	return wstr_get_at(line, conf.cx);
}

void line_insert(int at, const wchar_t *str, size_t len){
	WString *line = wstr_from_cwstr(str, len);
	vector_insert_at(conf.lines, at, &line);
	conf.num_lines++;
}

void line_append(const wchar_t *str, size_t len){
	WString *line = wstr_from_cwstr(str, len);
	vector_append(conf.lines, &line);
	conf.num_lines++;
}

int line_cx_to_rx(WString *line, int cx){
	int rx = 0;
	for (int i = 0; i < cx; i++){
		wchar_t c = wstr_get_at(line, i);
		rx += get_character_width(c, rx);
	}
	return rx;
}

void line_move_up(){
	if (conf.cy == 0) return;
	vector_swap(conf.lines, conf.cy, conf.cy - 1);
	conf.cy--;
	conf.dirty++;
}

void line_move_down(){
	if (conf.cy == conf.num_lines - 1) return;
	vector_swap(conf.lines, conf.cy, conf.cy + 1);
	conf.cy++;
	conf.dirty++;
}

void line_put_char(int c){
	if (conf.cy == conf.num_lines)
		line_insert(conf.num_lines, L"", 0);
	WString *line;
	vector_at(conf.lines, conf.cy, &line);
	int n = 1;
	if (c == L'\t' && conf.substitute_tab_with_space){
		n = get_character_width(L'\t', conf.cx);
		for (int i = 0; i < n; i++)
			wstr_insert(line, ' ', conf.cx);
	}else {
		wstr_insert(line, c, conf.cx);
	}
	conf.cx += n;
	conf.dirty += n;
}

void line_delete_char_forward(void){
	if (conf.cy == conf.num_lines)
		return;
	if (conf.cx == 0 && current_line_length() == 0){
		vector_remove_at(conf.lines, conf.cy);
		conf.num_lines--;
	}else{
		cursor_move(ARROW_RIGHT);
		line_delete_char_backwards();
	}
}

void line_delete_char_backwards(void){
	if (conf.cy == conf.num_lines){
		cursor_move(ARROW_LEFT);
		return;
	}
	WString *line;
	vector_at(conf.lines, conf.cy, &line);
	if (conf.cx == 0){
		if (conf.cy == 0)
			return;
		WString *up;
		vector_at(conf.lines, conf.cy - 1, &up);
		size_t new_x = wstr_length(up);
		wstr_concat_wstr(up, line);
		vector_remove_at(conf.lines, conf.cy);
		conf.num_lines--;
		cursor_move(ARROW_LEFT);
		conf.cx = new_x;
	}
	else if (conf.cx > 0 && (size_t)(conf.cx - 1) < wstr_length(line)){
		wstr_remove_at(line, conf.cx - 1);
		cursor_move(ARROW_LEFT);
	}
	conf.dirty++;
}

void line_delete_word_forward(void){
	if ((size_t)conf.cx == current_line_length()){
		line_delete_char_forward();
		return;
	}
	int x = conf.cx;
	cursor_jump_word(ARROW_RIGHT);
	int diff = conf.cx - x;
	while (diff-- > 0)
		line_delete_char_backwards();
}

void line_delete_word_backwards(void){
	if (conf.cx == 0){
		line_delete_char_backwards();
		return;
	}
	int x = conf.cx;
	cursor_jump_word(ARROW_LEFT);
	int diff = x - conf.cx;
	while (diff-- > 0)
		line_delete_char_forward();
}

void line_insert_newline(void){
	if (conf.cx == 0){
		line_insert(conf.cy, L"", 0);
	}else{
		WString *current;
		vector_at(conf.lines, conf.cy, &current);
		wchar_t *split = wstr_substring(current, conf.cx, wstr_length(current));
		size_t split_len = wstr_length(current) - conf.cx;
		wstr_remove_range(current, conf.cx, wstr_length(current));
		line_insert(conf.cy + 1, split, split_len);
		free(split);
	}
	conf.cy++;
	conf.cx = 0;
	conf.dirty++;
}

void line_cut(bool whole){
	if (conf.cy >= conf.num_lines)
		return;
	if (whole){
		vector_remove_at(conf.lines, conf.cy);
		conf.num_lines--;
		cursor_adjust();
	}else {
		WString *line = current_line();
		wstr_remove_range(line, conf.cx, -1);
	}
	conf.dirty++;
}

void line_toggle_comment(void){
	int mode = mode_get_current();
	if (mode == NO_MODE) return;
	const wchar_t *comment_start = mode_comments[mode][0];
	const wchar_t *comment_end = mode_comments[mode][1];

	WString *line = current_line();
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
	if (cy >= conf.num_lines)
		return;
	assert(cy >= 0);
	WString *line;
	vector_at(conf.lines, cy, &line);
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
		if (cy == conf.cy){
			if ((size_t)conf.cx >= removed){
				conf.cx -= removed;
			}else{
				conf.col_offset -= removed;
				if (conf.col_offset < 0){
					conf.cx += conf.col_offset;
					conf.col_offset = 0;
				}
			}
		}
	}
}

void line_format(int cy){
	line_strip_trailing_spaces(cy);
	wchar_t tab_buffer[80];
	swprintf(tab_buffer, 80, L"%*c", conf.tab_size, ' ');
	WString *line = line_at(cy);

	if (conf.substitute_tab_with_space){
		if (cy == conf.cy){
			int i = wstr_find_substring(line, L"\t", 0);
			while (i >= 0 && i < conf.cx){
				conf.cx += conf.tab_size - 1;
				i = wstr_find_substring(line, L"\t", i + 1);
			}
		}
		wstr_replace(line, L"\t", tab_buffer);
	}else{
		if (cy == conf.cy){
			int i = wstr_find_substring(line, tab_buffer, 0);
			while (i >= 0 && i < conf.cx){
				conf.cx -= conf.tab_size - 1;
				i = wstr_find_substring(line, tab_buffer, i + conf.tab_size);
			}
		}
		wstr_replace(line, tab_buffer, L"\t");
	}
}
