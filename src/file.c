#include "prelude.h"

#include "file.h"
#include "line.h"
#include "util.h"
#include "io.h"
#include "buffer.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <assert.h>

static LinkedList *history;

static void cleanup_file(void) {
        list_free(history);
}

void init_file(void) {
        CLEANUP_GUARD;
        history = list_init(sizeof(wchar_t*), compare_pointer);
        list_set_destructor(history, destroy_ptr);
        atexit(cleanup_file);
}

static char* mb_filename(size_t *written, bool tmp){
	static char mbfilename[NAME_MAX];
	static char full_filename[PATH_MAX];
	size_t wrt = wcstombs(mbfilename, buffers.curr->filename, NAME_MAX);
	mbfilename[NAME_MAX - 1] = '\0';

	char *last_slash = strrchr(mbfilename, '/');
	if (last_slash)
		*last_slash = '\0';

	if (mbfilename[0] == '/'){
		wrt += snprintf(full_filename, PATH_MAX,
				"%s%s%s%s",
				last_slash ? mbfilename : "",
				last_slash ? "/" : "",
				tmp ? "." : "",
				last_slash ? last_slash + 1 : mbfilename);

	}else{
		wrt += snprintf(full_filename, PATH_MAX,
				"%s/%s%s%s%s", editor_cwd(),
				last_slash ? mbfilename : "",
				last_slash ? "/" : "",
				tmp ? "." : "",
				last_slash ? last_slash + 1 : mbfilename);
	}

	if (written)
		*written = wrt;
	return full_filename;
}

static WString* editor_lines_to_string(void){
	size_t n_lines = vector_size(buffers.curr->lines);
	WString *str = wstr_empty();
	for (size_t i = 0; i < n_lines; i++){
		WString *line;
		vector_at(buffers.curr->lines, i, &line);
		wstr_concat_wstr(str, line);
		wstr_concat_cstr(str, buffers.curr->conf.eol, -1);
	}
	return str;
}

char* get_tmp_filename(void){
	if (!buffers.curr->filename)
		return NULL;
	size_t written;
	char *filename = mb_filename(&written, true);
	size_t tmp_filename_len = written + sizeof(TMP_EXT) + 1;
	char *tmp_filename = xmalloc(tmp_filename_len * sizeof(char));
	snprintf(tmp_filename, tmp_filename_len, "%s%s", filename, TMP_EXT);
	return tmp_filename;
}

static volatile int ctrl_c_pressed = 0;

static void ctrl_c_handler(int signal){
	if (signal == SIGINT)
		ctrl_c_pressed = 1;
}

static void switch_ctrl_c(bool allow){
	static struct termios old, new;
	if (allow){
		tcgetattr(0, &old);
		new = old;
		new.c_lflag |= (ISIG);
		tcsetattr(0, TCSANOW, &new);
		signal(SIGINT, ctrl_c_handler);
		ctrl_c_pressed = 0;
	}else{
		tcsetattr(0, TCSANOW, &old);
		signal(SIGINT, SIG_DFL);
	}
}

// TODO: if .tmp version exists (i.e. failed save in a previous session), restore it.
int _file_open(const wchar_t *filename) {
	if (!buffers.curr->filename || !filename || buffers.curr->filename != filename){
		size_t len = (wstrnlen(filename, FILENAME_MAX) + 1) * sizeof(wchar_t);
		free(buffers.curr->filename);
		buffers.curr->filename = xmalloc(len);
		assert(buffers.curr->filename);
		memcpy(buffers.curr->filename, filename, len);
	}

        char *mbfilename = mb_filename(NULL, false);
	FILE *f = fopen(mbfilename, "r");
	if (!f)
		return 1;

	WString *buf = wstr_empty();
	wint_t c = 1;
	switch_ctrl_c(true);
	while (c != WEOF){
		c = getwc(f);
		if (c == L'\n' || c == L'\r' || c == WEOF){
			if (c == L'\r'){
				buffers.curr->conf.eol = "\r\n";
				c = getwc(f); // consume the \n
			}else if (c == L'\n'){
				buffers.curr->conf.eol = "\n";
			}

			size_t len = wstr_length(buf);
			if (len != 0 || c == L'\n' || c == L'\r')
				line_insert(buffers.curr->num_lines, wstr_get_buffer(buf), len);
			wstr_clear(buf);
		}else{
			wstr_push_char(buf, c);
		}
		if (ctrl_c_pressed){
			switch_ctrl_c(false);
			return -1;
		}
	}
	switch_ctrl_c(false);
	wstr_free(buf);
	buffers.curr->dirty = 0;
	fclose(f);

        if (access(mbfilename, W_OK) != 0)
                editor_set_status_message(L"Warning! You have opened a READ ONLY file");

	// Discard any key press made while loading the file
	while (editor_read_key() != NO_KEY)
		;

	return 1;
}

int file_open(const wchar_t *filename) {
        if (filename)
                return _file_open(filename);
        WString *filename_wstr = editor_prompt(L"Open file", buffers.curr->filename, history);
        if (filename_wstr && wstr_length(filename_wstr) > 0){
                buffer_clear();
                filename = wstr_get_buffer(filename_wstr);
                if (_file_open(filename) != 1) {
                        buffer_drop();
                        return FAILURE;
                }
        }
        wstr_free(filename_wstr);
        return SUCCESS;
}

int file_save(bool only_tmp, bool ask_filename){
	if (ask_filename){
		WString *filename = editor_prompt(L"Save as", buffers.curr->filename, history);
		if (!filename || wstr_length(filename) == 0)
			return -1;
		free(buffers.curr->filename);
		buffers.curr->filename = wstr_to_cwstr(filename);
		wstr_free(filename);
	}

        /* Check if the file is writable. This is because we
           write to a temporary file and then rename it, which
           could cause the original file to be replaced. If the
           file does not exist it is fine to create a new one.*/
        char *mbfilename = mb_filename(NULL, false);
        if (access(mbfilename, F_OK) == 0    // file exists
            && access(mbfilename, W_OK) != 0 // is NOT writable
        ){
                editor_set_status_message(L"Can't save! You have no permissions");
                return -1;
        }

	if (!only_tmp){
		editor_set_status_message(L"Saving...");
		editor_refresh_screen(true);
	}

	char *tmp_filename = get_tmp_filename();
	FILE *f = fopen(tmp_filename, "w");
	if (!f){
		editor_set_status_message(L"Can't save! I/O error: %s", strerror(errno));
		free(tmp_filename);
		return -2;
	}

        int status = SUCCESS;
	WString *buf = editor_lines_to_string();
	size_t len = wstr_length(buf);
	fwprintf(f, L"%ls", wstr_get_buffer(buf));
	fflush(f);
	fclose(f);

	if (only_tmp) goto cleanup;

	char *filename = mb_filename(NULL, false);
	struct stat file_stat;
	// if the file already exists, adjust the permissions
	bool adjust_perms = access(filename, F_OK) == 0;
	mode_t perms = 0;
	if (adjust_perms){
		stat(filename, &file_stat);
		perms = file_stat.st_mode;
	}

	/* Since we write to a temporary file and then rename it to the actual
	   one, saving a symlink would overwrite it. So in that case, we need to
	   get the "real" filename before saving. */
	lstat(filename, &file_stat);
	if (S_ISLNK(file_stat.st_mode)){
		char link[PATH_MAX];
                if (readlink(filename, link, PATH_MAX-1) == -1) {
                        status = -3;
                        goto cleanup;
                }

		char *last_slash = strrchr(filename, '/');
		if (last_slash) ++last_slash;
		else last_slash = filename;
		*last_slash = '\0';

		size_t base_len = strlen(filename);
		link[PATH_MAX - 1 - base_len] = '\0';
		strncat(last_slash, link, PATH_MAX);
	}

	if (rename(tmp_filename, filename) != 0
	    || (adjust_perms && chmod(filename, perms) != 0)
		){
		editor_set_status_message(L"Can't save! I/O error: %s", strerror(errno));
                status = -4;
                goto cleanup;
	}
	char *magnitudes[] = {"bytes", "KiB", "MiB", "GiB"};
	double value = len;
	size_t magnitude = 0;
	while ((int)(value / 1024) > 0 && magnitude < sizeof(magnitudes)){
		magnitude++;
		value /= 1024;
	}

	if (magnitude > 0)
		editor_set_status_message(L"%.1f %s written to disk [%s]",
					  value, magnitudes[magnitude], filename);
	else
		editor_set_status_message(L"%.0f %s written to disk [%s]",
					  value, magnitudes[magnitude], filename);
	buffers.curr->dirty = 0;

cleanup:
	wstr_free(buf);
	free(tmp_filename);
	return status;
}

void file_reload(void){
	int cx = buffers.curr->cx;
	int cy = buffers.curr->cy;
	int row_offset = buffers.curr->row_offset;
	int col_offset = buffers.curr->col_offset;
	buffer_clear();
	file_open(buffers.curr->filename);
	if (cy + row_offset <= buffers.curr->num_lines){
		buffers.curr->cy = cy;
		buffers.curr->row_offset = row_offset;
		WString *line;
		vector_at(buffers.curr->lines, cy, &line);
		size_t len = wstr_length(line);
		if ((size_t)(cx + col_offset) <= len){
			buffers.curr->cx = cx;
			buffers.curr->col_offset = col_offset;
		}
	}
	editor_refresh_screen(false);
}

void file_auto_save(void){
	if (!buffers.curr->conf.auto_save)
		return;
	int buf_i = buffers.curr_index;

	if (buffers.curr->dirty && buffers.curr->filename && time(0) - buffers.curr->last_auto_save >= 60){
		for (int i = 0; i < buffers.amount; i++){
			buffer_switch(i);
			file_save(true, false);
		}
		buffers.curr->last_auto_save = time(0);
	}

	buffer_switch(buf_i);
}
