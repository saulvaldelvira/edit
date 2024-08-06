#include "output.h"
#include "buffer.h"
#include "lib/str/wstr.h"
#include "line.h"
#include "util.h"
#include "state.h"
#include "cursor.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

static WString *buf = NULL;

static void cleanup(void){
	wstr_free(buf);
}

static int check_window_size(void){
	int rows, cols;
	get_window_size(&rows, &cols);
	rows -= 2;
	if (rows != state.screen_rows || cols != state.screen_cols){
		for (int i = state.screen_rows; i < rows; i++){
			WString *wstr = wstr_empty();
			vector_append(state.render, &wstr);
		}
		state.screen_rows = rows;
		state.screen_cols = cols;
		cursor_adjust();
		return 1;
	}
	return 0;
}

static int num_width(int n){
        int max = 1;
        while (n >= 10){
                n /= 10;
                max++;
        }
        return max;
}

static void move_cursor(int x, int y, bool respect_linenr){
        if (buffers.curr->conf.line_number && respect_linenr)
                x += num_width(buffers.curr->num_lines) + 1;
        wchar_t move_cursor_buf[32];
        swprintf(move_cursor_buf,
                 ARRAY_SIZE(move_cursor_buf),
                 L"\x1b[%d;%dH",
                 y, x);
        wstr_concat_cwstr(buf, move_cursor_buf, ARRAY_SIZE(move_cursor_buf));
}

void editor_refresh_screen(bool only_status_bar){
	if (!buf){
		buf = wstr_empty();
		atexit(cleanup);
	}

	if (check_window_size() != 0)
		only_status_bar = false;

	wstr_concat_cwstr(buf, L"\x1b[?25l", 6);

	if (!only_status_bar){
		editor_update_render();
		wstr_concat_cwstr(buf, L"\x1b[H", 3);
		editor_draw_rows(buf);
	}else{
		// Move to the last two lines (status and message)
                move_cursor(0, state.screen_rows + 1, false);
	}

	editor_draw_status_bar(buf);
	editor_draw_message_bar(buf);
	// Restore cursor position
        move_cursor(buffers.curr->rx - buffers.curr->col_offset + 1,
                    buffers.curr->cy - buffers.curr->row_offset + 1, true);

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
	for (i = 0; i < state.screen_rows / 4; i++)
		wstr_concat_cwstr(buf, L"~\x1b[K\r\n", 6);

	for (wchar_t **line = welcome_messages; *line != NULL;){
		wchar_t *msg = *line;
		int welcome_len = wstrnlen(msg, -1);
		if (welcome_len > state.screen_cols - 2)
			welcome_len = state.screen_cols - BOTTOM_MENU_HEIGHT;
		int padding = (state.screen_cols - welcome_len) / 2;
                padding -= BOTTOM_MENU_HEIGHT;
		wstr_concat_cwstr(buf, L"~ ", 2);
		while (padding-- > 0)
			wstr_push_char(buf, L' ');
		wstr_concat_cwstr(buf, msg, welcome_len);
		line++;
		wstr_concat_cwstr(buf, L"\x1b[K\r\n", 5);
		if (++i >= state.screen_rows)
			break;
	}

	for (; i <= state.screen_rows; i++)
		wstr_concat_cwstr(buf, L"~\x1b[K\r\n", 7);
}

void editor_draw_rows(WString *buf){
	if (buffers.curr->num_lines == 0
	 && buffers.curr->filename == NULL
	 && buffers.curr->dirty == 0
	 && buffers.amount == 1
	 ){
		print_welcome_msg(buf);
		return;
	 }

	for (int y = 0; y < state.screen_rows; y++){
		int file_line = y + buffers.curr->row_offset;
		if (file_line >= buffers.curr->num_lines){
			wstr_concat_cwstr(buf, L"~", 1);
		}else{
                        size_t screen_cols = state.screen_cols;
                        if (buffers.curr->conf.line_number) {
                                screen_cols -= num_width(buffers.curr->num_lines) + 1;
                                wchar_t lnr_buf[128];
                                int padding = num_width(buffers.curr->num_lines) - num_width(file_line + 1) + 1;
                                swprintf(lnr_buf, sizeof(lnr_buf) / sizeof(lnr_buf[0]),
                                         L"%d%*s", file_line + 1, padding, "");
                                wstr_concat_cwstr(buf, lnr_buf, sizeof(lnr_buf));
                        }
                        WString *line;
                        vector_at(state.render, y, &line);
                        size_t len = wstr_length(line);
			if ((size_t)buffers.curr->col_offset < len){
				if (len >= (size_t)buffers.curr->col_offset)
					len -= buffers.curr->col_offset;
				if (len > screen_cols)
					len = screen_cols;
				const wchar_t *line_buf = wstr_get_buffer(line);
				wstr_concat_cwstr(buf, &line_buf[buffers.curr->col_offset], len);
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
			   buffers.curr->filename ? buffers.curr->filename : L"[Unnamed]",
			   buffers.curr->num_lines,
		       	   buffers.curr->dirty ? L"(modified)" : L"");
	int rlen = swprintf(rstatus, ARRAY_SIZE(rstatus),
			    L"Buffer:%d/%d | Row:%d | Col:%d ",
			    buffers.curr_index + 1, buffers.amount,
			    buffers.curr->cy + 1, buffers.curr->cx + 1);
	if (len > state.screen_cols)
		len = state.screen_cols;
	if (rlen > state.screen_cols - len)
		rlen = 0;

	int sep_len = state.screen_cols - len - rlen;
	swprintf(sep, ARRAY_SIZE(sep), L"%*s", sep_len, " ");

	wstr_concat_cwstr(buf, status, len);
	wstr_concat_cwstr(buf, sep, sep_len);
	wstr_concat_cwstr(buf, rstatus, rlen);;

	wstr_concat_cwstr(buf, L"\x1b[m", 3);
	wstr_concat_cwstr(buf, L"\r\n", 2);
}

void editor_draw_message_bar(WString *buf){
	size_t len = ARRAY_SIZE(state.status_msg);
	if (len > (size_t)state.screen_cols)
		len = (size_t)state.screen_cols;
	wstr_concat_cwstr(buf, L"\x1b[K", 3);
	if(time(NULL) - state.status_msg_time < 5)
		wstr_concat_cwstr(buf, state.status_msg, len);

}

void editor_set_status_message(const wchar_t *fmt, ...){
	va_list ap;
	va_start(ap, fmt);
	vswprintf(state.status_msg, ARRAY_SIZE(state.status_msg), fmt, ap);
	va_end(ap);
	state.status_msg_time = time(NULL);
}

void editor_help(void){
	buffer_insert();
        buffers.curr->conf.line_number = false;
        static wchar_t *lines[] = {
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
		L"Alt + C            \t Toggle comment in the current line",
		L"Alt + S            \t Search in the buffer",
		L"Alt + K            \t Cut the line from the cursor to the end",
		L"Alt + Up Arrow     \t Moves the current line up",
		L"Alt + Down Arrow   \t Moves the current line down",
		L"Alt + [1-9]        \t Go to buffer number [1-9]",
		L"Alt + Left Arrow   \t Jumps to the previous word ",
		L"Alt + Righ Arrow   \t Jumps to the next word ",
		L"Alt + Backspace    \t Deletes a word backwards ",
		L"Alt + Supr Key     \t Deletes a word forward ",
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
		L"replace <text> <replacement>  Replaces all instances of <text> with <replacement>",L"",
		L"goto [line|buffer] <number>   Jumps to a certain line/buffer",L"",
		L"format [line|buffer]	The same as Ctrl + F.",L"",
                L"set <key> <value> Set a state.guration parameter for all new buffers",L"",
                L"setlocal <key> <value> Set a state.guration parameter for the current buffer",L"",
		L"help	Display the help buffer",
		NULL
	};
	for (wchar_t **line = lines; *line != NULL; line++)
		line_append(*line, -1);
}
