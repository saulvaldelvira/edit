#include "prelude.h"
#include "state.h"
#include <string.h>
#include <sys/stat.h>
#include <wchar.h>
#include "util.h"
#include "buffer.h"
#include "buffer/highlight.h"
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

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

size_t wstrlen(const wchar_t *wstr){
	size_t len = 0;
	while (*wstr++ != '\0')
		len++;
	return len;
}

int get_cursor_position(int *row, int *col){
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

char* editor_cwd(void){
	static char cwd[PATH_MAX];
	if (getcwd(cwd, PATH_MAX) == NULL)
		die("Could not get CWD, on editor_cwd");
	return cwd;
}

static void update_line(int cy){
	int i = cy - buffers.curr->row_offset;
	string_t *render;
	vector_at(state.render, i, &render);
	str_clear(render);
	string_t *row;
	vector_at(buffers.curr->lines, cy, &row);
	int rx = 0;
	for (size_t j = 0; j < str_length_utf8(row); j++){
		wchar_t c = str_get_at(row, j);
		if (c == L'\t'){
			for (int i = 0; i < get_character_width(L'\t', rx); i++)
				str_push_char(render, ' ');
		}else{
			str_push_char(render, c);
		}
		rx += get_character_width(c, rx);
	}
}

void editor_update_render(void){
	int i;
	for (i = 0; i < state.screen_rows; i++){
		int cy = buffers.curr->row_offset + i;
		if (cy >= buffers.curr->num_lines)
			break;
		update_line(cy);
	}
	for (; i < state.screen_rows; i++){
		string_t *render;
		vector_at(state.render, i, &render);
		str_clear(render);
	}
	if (buffers.curr->conf.syntax_highlighting)
		editor_highlight();
}

int get_character_width(wchar_t c, int accumulated_rx){
	if (c == L'\t')
		return buffers.curr->conf.tab_size - (accumulated_rx % buffers.curr->conf.tab_size);
	else
#ifdef __WIN32
                return 1;
#else
		return wcwidth(c);
#endif

}

void free_str(void *e){
	str_free(*(string_t**)e);
}

long get_time_millis(void){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

char* dup_string(const char *str) {
        size_t len = strlen(str) + 1;
        char *new = xmalloc(len);
        strncpy(new, str, len);
        new[len - 1] = '\0';
        return new;
}

wchar_t* wstrdup(const wchar_t *wstr) {
        size_t len = wstrlen(wstr) + 1;
        wchar_t *dup = xmalloc(len * sizeof(wchar_t));
        memcpy(dup, wstr, len * sizeof(wchar_t));
        dup[len-1] = L'\0';
        return dup;
}
