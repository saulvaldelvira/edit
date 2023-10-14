#include "file.h"
#include "line.h"
#include "util.h"
#include "output.h"
#include "input.h"
#include "buffer.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <assert.h>

static char* mb_filename(size_t *written, bool tmp){
	static char mbfilename[NAME_MAX];
	static char full_filename[PATH_MAX];
	size_t wrt = wcstombs(mbfilename, conf.filename, NAME_MAX);
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
	size_t n_lines = vector_size(conf.lines);
	WString *str = wstr_empty();
	for (size_t i = 0; i < n_lines; i++){
		WString *line;
		vector_at(conf.lines, i, &line);
		wstr_concat_wstr(str, line);
		wstr_concat_cstr(str, conf.eol, -1);
	}
	return str;
}

char* get_tmp_filename(void){
	if (!conf.filename)
		return NULL;
	size_t written;
	char *filename = mb_filename(&written, true);
	size_t tmp_filename_len = written + sizeof(TMP_EXT) + 1;
	char *tmp_filename = malloc(tmp_filename_len * sizeof(char));
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
int file_open(const wchar_t *filename){
	if (!conf.filename || !filename || conf.filename != filename){
		size_t len = (wstrnlen(filename, FILENAME_MAX) + 1) * sizeof(wchar_t);
		free(conf.filename);
		conf.filename = malloc(len);
		assert(conf.filename);
		memcpy(conf.filename, filename, len);
	}

	FILE *f = fopen(mb_filename(NULL, false), "r");
	if (!f)
		return 1;

	WString *buf = wstr_empty();
	wint_t c = 1;
	switch_ctrl_c(true);
	while (c != WEOF){
		c = getwc(f);
		if (c == L'\n' || c == L'\r' || c == WEOF){
			if (c == L'\r'){
				conf.eol = "\r\n";
				c = getwc(f); // consume the \n
			}else if (c == L'\n'){
				conf.eol = "\n";
			}

			size_t len = wstr_length(buf);
			if (len != 0 || c == L'\n' || c == L'\r')
				line_insert(conf.num_lines, wstr_get_buffer(buf), len);
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
	conf.dirty = 0;
	fclose(f);

	// Discard any key press made while loading the file
	while (editor_read_key() != NO_KEY)
		;

	return 1;
}

// TODO: if finds ~, substitute for home tmp
int file_save(bool only_tmp, bool ask_filename){
	if (ask_filename){
		WString *filename = editor_prompt(L"Save as", conf.filename);
		if (!filename || wstr_length(filename) == 0){
			editor_set_status_message(L"Save aborted");
			return -1;
		}
		free(conf.filename);
		conf.filename = wstr_to_cwstr(filename);
		wstr_free(filename);
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

	WString *buf = editor_lines_to_string();
	size_t len = wstr_length(buf);
	fwprintf(f, L"%ls", wstr_get_buffer(buf));
	fflush(f);
	fclose(f);

	if (!only_tmp){
		char *filename = mb_filename(NULL, false);
		bool adjust_perms = access(filename, F_OK) == 0;
		mode_t perms;
		if (adjust_perms){
			struct stat file_stat;
			if (stat(filename, &file_stat) != 0)
				die("stat, on editor_save");
			perms = file_stat.st_mode;
		}
		if (rename(tmp_filename, filename) != 0
		    || (adjust_perms && chmod(filename, perms) != 0)
			){
			editor_set_status_message(L"Can't save! I/O error: %s", strerror(errno));
			wstr_free(buf);
		       	free(tmp_filename);
			return -3;
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
		conf.dirty = 0;
	}

	wstr_free(buf);
	free(tmp_filename);
	return 1;
}

void file_reload(void){
	int cx = conf.cx;
	int cy = conf.cy;
	int row_offset = conf.row_offset;
	int col_offset = conf.col_offset;
	buffer_clear();
	file_open(conf.filename);
	if (cy + row_offset <= conf.num_lines){
		conf.cy = cy;
		conf.row_offset = row_offset;
		WString *line;
		vector_at(conf.lines, cy, &line);
		size_t len = wstr_length(line);
		if ((size_t)(cx + col_offset) <= len){
			conf.cx = cx;
			conf.col_offset = col_offset;
		}
	}
	editor_refresh_screen(false);
}

void file_auto_save(void){
	if (!conf.auto_save)
		return;
	int buf_i = conf.buffer_index;

	if (conf.dirty && conf.filename && time(0) - conf.last_auto_save >= 60){
		for (int i = 0; i < conf.n_buffers; i++){
			buffer_switch(i);
			file_save(true, false);
		}
		conf.last_auto_save = time(0);
	}

	buffer_switch(buf_i);
}

