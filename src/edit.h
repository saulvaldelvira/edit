#ifndef EDIT_H
#define EDIT_H

#define _XOPEN_SOURCE // wcwidth

#include "./lib/GDS/src/Vector.h"
#include "./lib/str/wstr.h"
#include <termios.h>
#include <time.h>
#include <wchar.h>
#include <linux/limits.h>

#include "buffer.h"

#define VERSION "0.1"
#define CTRL_KEY(k) ((k) & 0x1f)
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#define TMP_EXT ".tmp"
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define DEFAULT_EOL "\n"

extern struct conf{
	// Current buffer's config
	int cx, cy;
	int rx;
	int row_offset;
	int col_offset;
	int num_lines;
	wchar_t *filename;
	Vector *lines;
	int dirty;
	char *eol;
	// General config
	int screen_rows;
	int screen_cols;
	struct termios original_term;
	int tab_size;
	int quit_times;
	bool substitute_tab_with_space;
	bool show_line_number;
	Vector *lines_render;
	Vector *buffers;
	int n_buffers;
	int buffer_index;
	wchar_t status_msg[160];
	time_t status_msg_time;
	bool auto_save;
	time_t last_auto_save;
	bool syntax_highlighting;
} conf;

#endif
