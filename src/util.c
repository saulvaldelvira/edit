#include "prelude.h"
#include "conf.h"
#include <wchar.h>
#include "util.h"
#include "buffer.h"
#include "buffer/highlight.h"
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void die(char *msg) {
        fprintf(stderr, "ERROR: %s. "
                        "In %s, line %d (%s)n",
                        msg, __FILE__, __LINE__, __func__);
        exit(1);
}                                                     \


void editor_end(void){
	for (int i = 0; i < buffers.amount; i++)
		buffer_drop();
	exit(0);
}

void* xmalloc(size_t nbytes){
        void *ptr = malloc(nbytes);
        if (!ptr)
                die("Could not allocate memory");
        return ptr;
}

size_t wstrnlen(const wchar_t *wstr, size_t n){
	size_t len = 0;
	while (*wstr++ != '\0' && n-- > 0)
		len++;
	return len;
}

void debug(const wchar_t *fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        vfwprintf(stderr, fmt, ap);
        va_end(ap);
}

static int get_cursor_position(int *row, int *col){
	wprintf(L"\x1b[6n\r\n");
	char buf[32];
	size_t i;
	for (i = 0; i < sizeof(buf) - 1; i++){
		if (read(STDIN_FILENO, &buf[i], 1) != 1)
			break;
		if (buf[1] == 'R')
			break;
	}
	buf[i] = '\0';

	if (buf[0] != '\x1b' || buf[1] != '[')
		return -1;
	if (sscanf(&buf[2], "%d;%d", row, col) != 2)
		return -1;
	return 0;
}

int get_window_size(int *rows, int *cols){
	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
		wprintf(L"\x1b[999C\x1b[999B");
		return get_cursor_position(rows, cols);
	}else{
		*rows = ws.ws_row;
		*cols = ws.ws_col;
		return 0;
	}
}

char* editor_cwd(void){
	static char cwd[PATH_MAX];
	if (getcwd(cwd, PATH_MAX) == NULL)
		die("Could not get CWD, on editor_cwd");
	return cwd;
}

static void update_line(int cy){
	int i = cy - buffers.curr->row_offset;
	WString *render;
	vector_at(conf.render, i, &render);
	wstr_clear(render);
	WString *row;
	vector_at(buffers.curr->lines, cy, &row);
	int rx = 0;
	for (size_t j = 0; j < wstr_length(row); j++){
		wchar_t c = wstr_get_at(row, j);
		if (c == L'\t'){
			for (int i = 0; i < get_character_width(L'\t', rx); i++)
				wstr_push_char(render, ' ');
		}else{
			wstr_push_char(render, c);
		}
		rx += get_character_width(c, rx);
	}
}

void editor_update_render(void){
	int i;
	for (i = 0; i < conf.screen_rows; i++){
		int cy = buffers.curr->row_offset + i;
		if (cy >= buffers.curr->num_lines)
			break;
		update_line(cy);
	}
	for (; i < conf.screen_rows; i++){
		WString *render;
		vector_at(conf.render, i, &render);
		wstr_clear(render);
	}
	if (buffers.curr->syntax_highlighting)
		editor_highlight();
}

int get_character_width(wchar_t c, int accumulated_rx){
	if (c == L'\t')
		return buffers.curr->tab_size - (accumulated_rx % buffers.curr->tab_size);
	else
		return wcwidth(c);

}

void free_wstr(void *e){
	wstr_free(*(WString**)e);
}

long get_time_millis(void){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}
