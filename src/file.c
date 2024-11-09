#include "prelude.h"
#include "definitions.h"
#include "lib/str/wstr.h"
#include "log.h"

#include "platform.h"
#include "file.h"
#include "buffer/line.h"
#include "state.h"
#include "util.h"
#include "console/io.h"
#include "buffer.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>
#include <signal.h>
#include <assert.h>
#include <init.h>
#include <platform.h>

static vector_t *history;

static void __cleanup_file(void) {
        CLEANUP_FUNC;
        IGNORE_ON_FAST_CLEANUP(
                vector_free(history);
        )
}

void init_file(void) {
        INIT_FUNC;
        history = vector_init(sizeof(wchar_t*), compare_pointer);
        vector_set_destructor(history, destroy_ptr);
        atexit(__cleanup_file);
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
		return FAILURE;

	wint_t c = 1;
	switch_ctrl_c(true);
        bool newline = false;
        while ((c = getwc(f)) != WEOF){
                if (newline) {
                        line_put_char('\n');
                        newline = false;
                }

                if (c == L'\n' || c == L'\r') {
                        if (c == L'\r'){
                                buffers.curr->conf.eol = "\r\n";
                                c = getwc(f); // consume the \n
                        }else if (c == L'\n'){
                                buffers.curr->conf.eol = "\n";
                        }
                        newline = true;
                } else {
                        line_put_char(c);
                }

                if (is_ctrl_c_pressed()) {
                        switch_ctrl_c(false);
                        return -1;
                }
        }
	switch_ctrl_c(false);
        fclose(f);

	buffers.curr->dirty = 0;
        cursor_goto_start();
        editor_set_status_message(L"");

        if (!file_writable(mbfilename))
                editor_set_status_message(L"Warning! You have opened a READ ONLY file");

	// Discard any key press made while loading the file
	while (editor_read_key().k != NO_KEY)
		;
        editor_log(LOG_INFO, "Opened file: %s", mbfilename);

	return SUCCESS;
}

int file_open(const wchar_t *filename) {
        if (!filename)
                filename = editor_prompt(L"Open file", buffers.curr->filename, history);
        editor_set_status_message(L"Opening \"%ls\"", filename);
        if (filename && wstrlen(filename) > 0){
                buffer_clear();
                if (_file_open(filename) != 1) {
                        buffer_drop(true);
                        editor_set_status_message(L"");
                        return FAILURE;
                }
        }
        return SUCCESS;
}

static void __print_filesave(double len, char *filename, unsigned long n_lines, bool auto_save) {
	char *magnitudes[] = {"bytes", "KiB", "MiB", "GiB"};
	double value = len;
	size_t magnitude = 0;
	while ((int)(value / 1024) > 0 && magnitude < sizeof(magnitudes)){
		magnitude++;
		value /= 1024;
	}
        int n_decimal = magnitude > 0 ? 1 : 0;
        char *auto_save_msg = auto_save ? "AUTO SAVE: " : "";
        if (!auto_save) {
                editor_set_status_message(L" %lluL %s%.*f %s written to: %s", n_lines, auto_save_msg,
                                n_decimal, value, magnitudes[magnitude], filename);
        }
        editor_log(LOG_INFO, " %lluL %s%.*f %s written to: %s", n_lines, auto_save_msg,
                             n_decimal, value, magnitudes[magnitude], filename);
}

static int __save_to(char *fname, size_t *len) {
	FILE *f = fopen(fname, "w");
	if (!f){
		editor_set_status_message(L"Can't save! I/O error: %s", strerror(errno));
		return -2;
	}
	size_t n_lines = vector_size(buffers.curr->lines);
        size_t __len = n_lines * strlen(buffers.curr->conf.eol);
	for (size_t i = 0; i < n_lines; i++){
		wstring_t *line = NULL;
		vector_at(buffers.curr->lines, i, &line);
                __len += wstr_length(line);
                assert(line);
                fprintf(f, "%ls%s", wstr_get_buffer(line), buffers.curr->conf.eol);
	}
        if (len)
	        *len = __len;
	fflush(f);
	fclose(f);
        return SUCCESS;
}

int file_save(bool only_tmp, bool ask_filename){
	if (ask_filename){
		const wchar_t *filename = editor_prompt(L"Save as", buffers.curr->filename, history);
		if (!filename || wstrlen(filename) == 0)
			return -1;
                change_current_buffer_filename(wstrdup(filename));
        }

        if (!buffers.curr->filename) {
                editor_log(LOG_WARN, "FILE: Attempted to save to a buffer without a filename");
                return -2;
        }

        /* Check if the file is writable. This is because we
           write to a temporary file and then rename it, which
           could cause the original file to be replaced. If the
           file does not exist it is fine to create a new one.*/
        char *mbfilename = mb_filename(NULL, false);
        if (file_exists(mbfilename)    // file exists
            && !file_writable(mbfilename) // is NOT writable
        ){
                if (!only_tmp)
                        editor_set_status_message(L"Can't save! You have no permissions");
                return -1;
        }

	if (!only_tmp){
		editor_set_status_message(L"Saving...");
	}

	char *tmp_filename = get_tmp_filename();
	/* FILE *f = fopen(tmp_filename, "w"); */
	/* if (!f){ */
	/* 	editor_set_status_message(L"Can't save! I/O error: %s", strerror(errno)); */
	/* 	free(tmp_filename); */
	/* 	return -2; */
	/* } */
        size_t len = 0;
        int status = __save_to(tmp_filename, &len);
        if (status != SUCCESS)
                goto cleanup;

	/* wstring_t *buf = editor_lines_to_string(); */
	/* size_t len = wstr_length(buf); */
	/* fwprintf(f, L"%ls", wstr_get_buffer(buf)); */
	/* fflush(f); */
	/* fclose(f); */


	if (only_tmp) {
                __print_filesave(len, tmp_filename, buffers.curr->num_lines, true);
                goto cleanup;
        }

	char *filename = mb_filename(NULL, false);
	// if the file already exists, adjust the permissions
	bool adjust_perms = file_exists(filename);
        long perms = 0;
	if (adjust_perms){
		perms = get_file_mode(filename);
	}

	/* Since we write to a temporary file and then rename it to the actual
	   one, saving a symlink would overwrite it. So in that case, we need to
	   get the "real" filename before saving. */
	if (is_link(filename)) {
		char link[PATH_MAX];
                if (!read_link(filename, link, PATH_MAX-1)) {
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
	    || (adjust_perms && !change_mod(filename, perms))
		){
                if (!only_tmp)
                        editor_set_status_message(L"Can't save! I/O error: %s", strerror(errno));
                status = -4;
                goto cleanup;
	}

        __print_filesave(len, filename, buffers.curr->num_lines, false);
	buffers.curr->dirty = 0;

cleanup:
	free(tmp_filename);
	return status;
}

void file_reload(void){
	int cx = buffers.curr->cx;
	int cy = buffers.curr->cy;
	int row_offset = buffers.curr->row_offset;
	int col_offset = buffers.curr->col_offset;
	buffer_clear();
        if (!buffers.curr->filename)
                return;
	file_open(buffers.curr->filename);
	if (cy + row_offset <= buffers.curr->num_lines){
		buffers.curr->cy = cy;
		buffers.curr->row_offset = row_offset;
		wstring_t *line;
		vector_at(buffers.curr->lines, cy, &line);
		size_t len = wstr_length(line);
		if ((size_t)(cx + col_offset) <= len){
			buffers.curr->cx = cx;
			buffers.curr->col_offset = col_offset;
		}
	}
}

void file_auto_save(void){
        time_t t = buffers.curr->conf.auto_save_interval;
	if (t == 0)
		return;
	int buf_i = buffers.curr_index;

	if (buffers.curr->dirty && buffers.curr->filename && time(0) - buffers.curr->last_auto_save >= t){
		for (int i = 0; i < buffers.amount; i++){
			buffer_switch(i);
			file_save(true, false);
		}
		buffers.curr->last_auto_save = time(0);
	}

	buffer_switch(buf_i);
}
