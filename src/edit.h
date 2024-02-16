#ifndef EDIT_H
#define EDIT_H

#define _POSIX_C_SOURCE 200112L
#define _XOPEN_SOURCE // wcwidth

#include "./lib/GDS/src/Vector.h"
#include "./lib/str/wstr.h"
#include <termios.h>
#include <time.h>
#include <wchar.h>
#include <limits.h>
#include "buffer.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#define TMP_EXT ".tmp"
#define DEFAULT_EOL "\n"
#define MIN(a,b) ((a) < (b) ? (a) : (b))

extern struct conf{
	int screen_rows;
	int screen_cols;
	struct termios original_term;
	int tab_size;
	int quit_times;
	bool substitute_tab_with_space;
	bool show_line_number;
	Vector *render;
	wchar_t status_msg[160];
	time_t status_msg_time;
	bool auto_save;
	time_t last_auto_save;
	bool syntax_highlighting;
        bool line_number;
} conf;

#endif
