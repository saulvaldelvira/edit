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

static void print_welcome_msg(WString *buf){
	static wchar_t* welcome_messages[] = {
		L"Edit -- Terminal-based text editor",
		L"Press Alt + H to display the help buffer",
		NULL
	};
	int i;
	for (i = 0; i < conf.screen_rows / 4; i++)
		wstr_concat_cwstr(buf, L"~\x1b[K\r\n", 6);

	for (wchar_t **line = welcome_messages; *line != NULL;){
		wchar_t *msg = *line;
		int welcome_len = wstrnlen(msg, -1);
		if (welcome_len > conf.screen_cols - 2)
			welcome_len = conf.screen_cols - 2;
		int padding = (conf.screen_cols - welcome_len) / 2;
		wstr_concat_cwstr(buf, L"~ ", 2);
		padding-=2;
		while (padding-- > 0)
			wstr_push_char(buf, L' ');
		wstr_concat_cwstr(buf, msg, welcome_len);
		line++;
		wstr_concat_cwstr(buf, L"\x1b[K\r\n", 5);
		if (++i >= conf.screen_rows)
			break;
	}

	for (; i <= conf.screen_rows; i++)
		wstr_concat_cwstr(buf, L"~\x1b[K\r\n", 7);
}

void editor_draw_rows(WString *buf){
	if (conf.num_lines == 0
	 && conf.filename == NULL
	 && conf.dirty == 0
	 && conf.n_buffers == 1
	 ){
		print_welcome_msg(buf);
		return;
	 }

	for (int y = 0; y < conf.screen_rows; y++){
		int file_line = y + conf.row_offset;
		if (file_line >= conf.num_lines){
			wstr_concat_cwstr(buf, L"~", 1);
		}else{
			WString *line;
			vector_at(conf.lines_render, y, &line);
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

	wchar_t status[256], rstatus[256], sep[256];
	int len = swprintf(status, ARRAY_SIZE(status),
			   L" %ls - %d lines %ls",
			   conf.filename ? conf.filename : L"[Unnamed]",
			   conf.num_lines,
		       	   conf.dirty ? L"(modified)" : L"");
	int rlen = swprintf(rstatus, ARRAY_SIZE(rstatus),
			    L"Buffer:%d/%d | Row:%d | Col:%d ",
			    conf.buffer_index + 1, conf.n_buffers,
			    conf.cy + 1, conf.cx + 1);
	if (len > conf.screen_cols)
		len = conf.screen_cols;
	if (rlen > conf.screen_cols - len)
		rlen = 0;

	int sep_len = conf.screen_cols - len - rlen;
	swprintf(sep, ARRAY_SIZE(sep), L"%*s", sep_len, " ");

	wstr_concat_cwstr(buf, status, len);
	wstr_concat_cwstr(buf, sep, sep_len);
	wstr_concat_cwstr(buf, rstatus, rlen);;

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

void editor_set_status_message(const wchar_t *fmt, ...){
	va_list ap;
	va_start(ap, fmt);
	vswprintf(conf.status_msg, ARRAY_SIZE(conf.status_msg), fmt, ap);
	va_end(ap);
	conf.status_msg_time = time(NULL);
}

void editor_help(void){
	buffer_insert();
	static wchar_t* lines[] = {
		L"Keybindings",
		L"===========",
		L"Ctrl + E           \t Execute command",
		L"Ctrl + F           \t Format line",
		L"Ctrl + K           \t Cut line",
		L"Ctrl + N           \t Add buffer",
		L"Ctrl + O           \t Open file",
		L"Ctrl + Q           \t Kill buffer",
		L"Ctrl + S           \t Save buffer",
		L"Ctrl + Left Arrow  \t Move to the buffer on the left",
		L"Ctrl + Right Arrow \t Move to the buffer on the right",
		L"F5                 \t Reload file",
		L"Alt + H            \t Display help buffer",
		L"Alt + 7            \t Toggle comment in the current line",
		L"",
		L"Commands",
		L"========",
		L"!quit	Exit the editor",L"",
		L"pwd	prints the current working directory",L"",
		L"wq	Write the buffer and close it",L"",
		L"fwq	Same as wq, but it formats the buffer first",L"",
		L"strip [line|buffer] 	Strips trailing whitespaces",L"",
		L"search <string> 	jumps to the next occurence of the given string of text",
		L"  search-backwards	same as search, but backwards.",L"",
		L"goto [line|buffer] <number>	Jumps to a certain line/buffer",L"",
		L"format [line|buffer]	The same as Ctrl + F.",L"",
		L"help	Display the help buffer",
		NULL
	};
	for (wchar_t **line = lines; *line != NULL; line++)
		line_append(*line, -1);
}
