#include "prelude.h"
#include "cursor.h"
#include "state.h"
#include "io/input.h"
#include <buffer/line.h>

static void __cursor_page_down(void);
static void __cursor_page_up(void);

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

int cursor_move(cursor_direction_t direction){
	wstring_t *row = current_line();

	switch (direction){
        case CURSOR_DIRECTION_LEFT:
		if (buffers.curr->cx > 0){
			buffers.curr->cx--;
		}else if (buffers.curr->cy > 0){
			buffers.curr->cy--;
			buffers.curr->cx = current_line_length();
		}
		break;
	case CURSOR_DIRECTION_RIGHT:
		if (row && (size_t)buffers.curr->cx < wstr_length(row)){
			buffers.curr->cx++;
		}else if (row && (size_t)buffers.curr->cx == wstr_length(row)){
			if (buffers.curr->cy >= buffers.curr->num_lines - 1)
				return -1;
			buffers.curr->cy++;
			buffers.curr->cx = 0;
		}
		break;
	case CURSOR_DIRECTION_UP:
		if (buffers.curr->cy > 0)
			buffers.curr->cy--;
		break;
	case CURSOR_DIRECTION_DOWN:
		if (buffers.curr->cy < buffers.curr->num_lines - 1)
			buffers.curr->cy++;
		break;
	case CURSOR_DIRECTION_START:
		buffers.curr->cx = 0;
		break;
	case CURSOR_DIRECTION_END:
		buffers.curr->cx = current_line_length();
		break;
        case CURSOR_DIRECTION_PAGE_UP:
                __cursor_page_up();
                break;
        case CURSOR_DIRECTION_PAGE_DOWN:
                __cursor_page_down();
                break;
	}
	cursor_adjust();
	return 1;
}

static void __cursor_page_up(void) {
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
}

static void __cursor_page_down(void) {
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

int cursor_jump_word(cursor_direction_t dir){
        if (dir != CURSOR_DIRECTION_LEFT && dir != CURSOR_DIRECTION_RIGHT)
                return -1;
	cursor_move(dir);
	wchar_t startc = line_curr_char();
	bool startwhite = (startc == ' ' || startc == '\t');
	for (;;) {
		wchar_t c = line_curr_char();
		if (!startwhite && (c == ' ' || c == '\t')) break;
		if (startwhite && c != ' ' && c != '\t') break;
		if ((size_t)buffers.curr->cx == current_line_length()) return SUCCESS;
		if (buffers.curr->cx == 0) return SUCCESS;
		cursor_move(dir);
	}
	if (dir == CURSOR_DIRECTION_LEFT)
		cursor_move(CURSOR_DIRECTION_RIGHT);
        return SUCCESS;
}
