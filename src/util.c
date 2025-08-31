#include "console/cursor.h"
#include "prelude.h"
#include "fs.h"
#include "state.h"
#include <stddef.h>
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

void free_wstr(void *e){
	wstr_free(*(wstring_t**)e);
}

long get_time_millis(void){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

wchar_t* wstrdup(const wchar_t *wstr) {
        size_t len = wstrlen(wstr) + 1;
        wchar_t *dup = xmalloc(len * sizeof(wchar_t));
        memcpy(dup, wstr, len * sizeof(wchar_t));
        dup[len-1] = L'\0';
        return dup;
}

#include <assert.h>
static FILE* __get_file_from_sub_path(char *sub_path, char *mode) {
        char *data_dir = get_data_directory();
        size_t len = strlen(data_dir) + strlen(sub_path) + 2;
        char *full_path = malloc(len);

        snprintf(full_path, len, "%s/%s", data_dir, sub_path);
        full_path[len - 1] = '\0';

        FILE *f = fopen_mkdir(full_path, mode);

        free(full_path);

        return f;
}

void save_history_to_file(char *sub_path, vector_t *entries) {
        FILE *f = __get_file_from_sub_path(sub_path, "w");
        if (!f)
                return;

        for (size_t i = 0; i < vector_size(entries); i++) {
                wchar_t *line;
                vector_at(entries, i, &line);
                fprintf(f, "%ls\n", line);
        }

        fclose(f);
}

static wchar_t* strtowstr(char *str) {
        if (!str)
                return NULL;
        size_t len = mbstowcs(NULL, str, 0);
        if (len == (size_t) -1)
                return NULL;
        wchar_t *wstr = malloc((len + 1) * sizeof(wchar_t));
        mbstowcs(wstr, str, len);
        wstr[len] = '\0';
        return wstr;
}

vector_t* load_history_from_file(char *sub_path) {
        vector_t *vec = vector_init(sizeof(wchar_t*), compare_pointer);
        vector_set_destructor(vec, destroy_ptr);

        FILE *f = __get_file_from_sub_path(sub_path, "r");
        if (!f)
                return vec;

        char *line = NULL;
        size_t len = 0;
        ssize_t read;

        while ( ( read = getline(&line, &len, f) ) != -1) {
                line[read - 1] = '\0';
                wchar_t *wstrline = strtowstr(line);
                vector_append(vec, &wstrline);
        }
        free(line);

        fclose(f);
        return vec;
}
