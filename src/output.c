#include "output.h"
#include "line.h"
#include "util.h"
#include "cursor.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static WString *buf = NULL;

static void cleanup(void){
	wstr_free(buf);
}

static int check_window_size(void){
	int rows, cols;
	get_window_size(&rows, &cols);
	rows -= 2;
	if (rows != conf.screen_rows || cols != conf.screen_cols){
		for (int i = conf.screen_rows; i < rows; i++){
			WString *wstr = wstr_empty();
			vector_append(conf.lines_render, &wstr);
		}
		conf.screen_rows = rows;
		conf.screen_cols = cols;
		cursor_scroll();
		return 1;
	}
	return 0;
}

void editor_refresh_screen(bool only_status_bar){
	if (!buf){
		buf = wstr_empty();
		atexit(cleanup);
	}

	if (check_window_size() != 0)
		only_status_bar = false;

	wchar_t move_cursor_buf[32];
	wstr_concat_cwstr(buf, L"\x1b[?25l", 6);

	if (!only_status_bar){
		editor_update_render();
		wstr_concat_cwstr(buf, L"\x1b[H", 3);
		editor_draw_rows(buf);
	}else{
		// Move to the last two lines (status and message)
		swprintf(move_cursor_buf,
			ARRAY_SIZE(move_cursor_buf),
			L"\x1b[%d;%dH",
			conf.screen_rows + 1,
			0);
		wstr_concat_cwstr(buf, move_cursor_buf, ARRAY_SIZE(move_cursor_buf));
	}

	editor_draw_status_bar(buf);
	editor_draw_message_bar(buf);
	// Restore cursor position
	swprintf(move_cursor_buf,
		 ARRAY_SIZE(move_cursor_buf),
		 L"\x1b[%d;%dH",
		 conf.cy - conf.row_offset + 1,
		 conf.rx - conf.col_offset + 1);
	wstr_concat_cwstr(buf, move_cursor_buf, ARRAY_SIZE(move_cursor_buf));

	wstr_concat_cwstr(buf, L"\x1b[?25h", 6);
	wprintf(L"%ls", wstr_get_buffer(buf));
	fflush(stdout);
	wstr_clear(buf);
}

void editor_draw_rows(WString *buf){
	for (int y = 0; y < conf.screen_rows; y++){
		int file_line = y + conf.row_offset;
		if (file_line >= conf.num_lines){
			if (conf.num_lines == 0 && conf.filename == NULL && conf.dirty == 0 /*&& conf.n_buffers == 1*/ && y == conf.screen_rows / 4){
				wchar_t welcome[80];
				int welcome_len = swprintf(welcome, ARRAY_SIZE(welcome),
							   L"Edit -- version %s", VERSION);
				if (welcome_len > conf.screen_cols)
					welcome_len = conf.screen_cols;
				int padding = (conf.screen_cols - welcome_len) / 2;
				if (padding){
					wstr_concat_cwstr(buf, L"~", 1);
					padding--;
				}
				while (padding--)
					wstr_concat_cwstr(buf, L" ", 1);
				wstr_concat_cwstr(buf, welcome, welcome_len);
			}else{
				wstr_concat_cwstr(buf, L"~", 1);
			}
		}else{
			WString *line;
			vector_get_at(conf.lines_render, y, &line);
			size_t len = wstr_length(line);
			if ((size_t)conf.col_offset < len){
				if (len >= (size_t)conf.col_offset)
					len -= conf.col_offset;
				if (len > (size_t)conf.screen_cols)
					len = (size_t)conf.screen_cols;
				const wchar_t *line_buf = wstr_get_buffer(line);
				wstr_concat_cwstr(buf, &line_buf[conf.col_offset], len);
			}
		}

		wstr_concat_cwstr(buf, L"\x1b[K\r\n", 5);
	}
}

void editor_draw_status_bar(WString *buf){
	wstr_concat_cwstr(buf, L"\x1b[7m", 4);

	wchar_t status[80], rstatus[80];
	int len = swprintf(status, ARRAY_SIZE(status),
			   L" %ls - %d lines %ls",
			   conf.filename ? conf.filename : L"[Unnamed]",
			   conf.num_lines,
		       	   conf.dirty ? L"(modified)" : L"");
	int rlen = swprintf(rstatus, ARRAY_SIZE(rstatus),
			    L"Buffer:%d/%d | Row:%d | Col:%d ", conf.buffer_index + 1, conf.n_buffers, conf.cy + 1, conf.cx + 1);
	if (len > conf.screen_cols)
		len = conf.screen_cols;
	wstr_concat_cwstr(buf, status, len);

	while (len < conf.screen_cols){
		if (conf.screen_cols - len == rlen){
			wstr_concat_cwstr(buf, rstatus, rlen);
			break;
		}else{
			wstr_push_char(buf, L' ');
			len++;
		}
	}

	wstr_concat_cwstr(buf, L"\x1b[m", 3);
	wstr_concat_cwstr(buf, L"\r\n", 2);
}

void editor_draw_message_bar(WString *buf){
	size_t len = ARRAY_SIZE(conf.status_msg);
	if (len > (size_t)conf.screen_cols)
		len = (size_t)conf.screen_cols;
	wstr_concat_cwstr(buf, L"\x1b[K", 3);
	if(time(NULL) - conf.status_msg_time < 5)
		wstr_concat_cwstr(buf, conf.status_msg, len);

}

// TODO: set with a specific timeout
// TODO: set in a Queue, and if one times out, check if another is ready
void editor_set_status_message(const wchar_t *fmt, ...){
	va_list ap;
	va_start(ap, fmt);
	vswprintf(conf.status_msg, ARRAY_SIZE(conf.status_msg), fmt, ap);
	va_end(ap);
	conf.status_msg_time = time(NULL);
}
