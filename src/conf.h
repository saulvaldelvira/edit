#ifndef CONF_H
#define CONF_H

#include "./lib/GDS/src/Vector.h"
#include "./lib/str/wstr.h"
#include <time.h>

extern struct conf{
	int screen_rows;
	int screen_cols;
	int quit_times;
	Vector *render;
	wchar_t status_msg[160];
	time_t status_msg_time;
} conf;

#endif // CONF_H
