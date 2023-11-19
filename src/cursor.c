#include "cursor.h"
#include "edit.h"
#include "input.h"
#include "line.h"
#include "util.h"

void cursor_adjust(void){
	size_t line_len = current_line_length();
	if ((size_t)conf.cx > line_len)
		conf.cx = line_len;
 	conf.rx = 0;
	if (conf.cy < conf.num_lines){
		WString *line;
		vector_at(conf.lines, conf.cy, &line);
		conf.rx = line_cx_to_rx(line, conf.cx);
	}
	// Adjust scroll
	if (conf.cy < conf.row_offset)
		conf.row_offset = conf.cy;
	if (conf.cy >= conf.row_offset + conf.screen_rows)
		conf.row_offset = conf.cy - conf.screen_rows + 1;
	if (conf.rx < conf.col_offset)
		conf.col_offset = conf.rx;
	if (conf.rx >= conf.col_offset + conf.screen_cols)
		conf.col_offset = conf.rx - conf.screen_cols + 1;
}


int cursor_move(int key){
	WString *row = current_line();

	switch (key){
	case ARROW_LEFT:
		if (conf.cx > 0){
			conf.cx--;
		}else if (conf.cy > 0){
			conf.cy--;
			conf.cx = current_line_length();
		}
		break;
	case ARROW_RIGHT:
		if (row && (size_t)conf.cx < wstr_length(row)){
			conf.cx++;
		}else if (row && (size_t)conf.cx == wstr_length(row)){
			if (conf.cy >= conf.num_lines)
				return -1;
			conf.cy++;
			conf.cx = 0;
		}
		break;
	case ARROW_UP:
		if (conf.cy > 0)
			conf.cy--;
		break;
	case ARROW_DOWN:
		if (conf.cy < conf.num_lines)
			conf.cy++;
		break;
	case PAGE_UP:
		if (conf.cy > conf.row_offset){
			conf.cy = conf.row_offset;
		}else{
			conf.cy -= conf.screen_rows;
			conf.row_offset -= conf.screen_rows;
			if (conf.cy < 0)
				conf.cy = 0;
			if (conf.row_offset < 0)
				conf.row_offset = 0;
		}
		break;
	case PAGE_DOWN:
		if (conf.cy < conf.row_offset + conf.screen_rows - 1){
			conf.cy = conf.row_offset + conf.screen_rows - 1;
			if (conf.cy >= conf.num_lines)
				conf.cy = conf.num_lines - 1;
		}else{
			conf.cy += conf.screen_rows;
			conf.row_offset += conf.screen_rows;
			if (conf.cy >= conf.num_lines)
				conf.cy = conf.num_lines - 1;
			if (conf.row_offset >= conf.num_lines)
				conf.row_offset = conf.num_lines - 1;

		}
		break;
	case HOME_KEY:
		conf.cx = 0;
		break;
	case END_KEY:
		conf.cx = current_line_length();
		break;
	}
	cursor_adjust();
	return 1;
}

void cursor_goto(int x, int y){
	conf.cy = y;
	conf.cx = x;
	if (y < conf.row_offset || y >= (conf.row_offset + conf.screen_rows)){
		if (y <= conf.screen_rows)
			conf.row_offset = 0;
		else
			conf.row_offset = y - conf.screen_rows / 2;
	}
	size_t len = current_line_length();
	if ((size_t)conf.cx > len)
		conf.cx = (int)len;
}
