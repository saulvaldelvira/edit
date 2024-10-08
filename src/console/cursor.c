#include "prelude.h"
#include "cursor.h"
#include "state.h"
#include "io/input.h"
#include <buffer/line.h>

void cursor_adjust(void){
        size_t line_len = current_line_length();
        if ((size_t)buffers.curr->cx > line_len)
                buffers.curr->cx = line_len;
        buffers.curr->rx = 0;
        if (buffers.curr->cy < buffers.curr->num_lines){
                wstring_t *line;
                vector_at(buffers.curr->lines, buffers.curr->cy, &line);
                buffers.curr->rx = line_cx_to_rx(line, buffers.curr->cx);
        }
        // Adjust scroll
        if (buffers.curr->cy < buffers.curr->row_offset)
                buffers.curr->row_offset = buffers.curr->cy;
        if (buffers.curr->cy >= buffers.curr->row_offset + state.screen_rows)
                buffers.curr->row_offset = buffers.curr->cy - state.screen_rows + 1;
        if (buffers.curr->rx < buffers.curr->col_offset)
                buffers.curr->col_offset = buffers.curr->rx;
        if (buffers.curr->rx >= buffers.curr->col_offset + state.screen_cols)
                buffers.curr->col_offset = buffers.curr->rx - state.screen_cols + 1;
}


int cursor_move(int key){
	wstring_t *row = current_line();

	switch (key){
        case CTRL_KEY('h'):
	case ARROW_LEFT:
		if (buffers.curr->cx > 0){
			buffers.curr->cx--;
		}else if (buffers.curr->cy > 0){
			buffers.curr->cy--;
			buffers.curr->cx = current_line_length();
		}
		break;
        case CTRL_KEY('l'):
	case ARROW_RIGHT:
		if (row && (size_t)buffers.curr->cx < wstr_length(row)){
			buffers.curr->cx++;
		}else if (row && (size_t)buffers.curr->cx == wstr_length(row)){
			if (buffers.curr->cy >= buffers.curr->num_lines)
				return -1;
			buffers.curr->cy++;
			buffers.curr->cx = 0;
		}
		break;
        case CTRL_KEY('k'):
	case ARROW_UP:
		if (buffers.curr->cy > 0)
			buffers.curr->cy--;
		break;
        case CTRL_KEY('j'):
	case ARROW_DOWN:
		if (buffers.curr->cy < buffers.curr->num_lines)
			buffers.curr->cy++;
		break;
	case PAGE_UP:
		if (buffers.curr->cy > buffers.curr->row_offset){
			buffers.curr->cy = buffers.curr->row_offset;
		}else{
			buffers.curr->cy -= state.screen_rows;
			buffers.curr->row_offset -= state.screen_rows;
			if (buffers.curr->cy < 0)
				buffers.curr->cy = 0;
			if (buffers.curr->row_offset < 0)
				buffers.curr->row_offset = 0;
		}
		break;
	case PAGE_DOWN:
		if (buffers.curr->cy < buffers.curr->row_offset + state.screen_rows - 1){
			buffers.curr->cy = buffers.curr->row_offset + state.screen_rows - 1;
			if (buffers.curr->cy >= buffers.curr->num_lines)
				buffers.curr->cy = buffers.curr->num_lines - 1;
		}else{
			buffers.curr->cy += state.screen_rows;
			buffers.curr->row_offset += state.screen_rows;
			if (buffers.curr->cy >= buffers.curr->num_lines)
				buffers.curr->cy = buffers.curr->num_lines - 1;
			if (buffers.curr->row_offset >= buffers.curr->num_lines)
				buffers.curr->row_offset = buffers.curr->num_lines - 1;

		}
		break;
	case HOME_KEY:
		buffers.curr->cx = 0;
		break;
	case END_KEY:
		buffers.curr->cx = current_line_length();
		break;
	}
	cursor_adjust();
	return 1;
}

void cursor_goto(int x, int y){
	buffers.curr->cy = y;
	buffers.curr->cx = x;
	if (y < buffers.curr->row_offset || y >= (buffers.curr->row_offset + state.screen_rows)){
		if (y <= state.screen_rows)
			buffers.curr->row_offset = 0;
		else
			buffers.curr->row_offset = y - state.screen_rows / 2;
	}
	size_t len = current_line_length();
	if ((size_t)buffers.curr->cx > len)
		buffers.curr->cx = (int)len;
}

void cursor_jump_word(int key){
	cursor_move(key);
	wchar_t startc = line_curr_char();
	bool startwhite = (startc == ' ' || startc == '\t');
	for (;;) {
		wchar_t c = line_curr_char();
		if (!startwhite && (c == ' ' || c == '\t')) break;
		if (startwhite && c != ' ' && c != '\t') break;
		if ((size_t)buffers.curr->cx == current_line_length()) return;
		if (buffers.curr->cx == 0) return;
		cursor_move(key);
	}
	if (key == ARROW_LEFT)
		cursor_move(ARROW_RIGHT);
}
