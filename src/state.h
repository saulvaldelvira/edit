#ifndef STATE_H
#define STATE_H

#include "./lib/GDS/src/Vector.h"
#include <time.h>

extern struct state {
	int screen_rows;
	int screen_cols;
	Vector *render;
	wchar_t status_msg[160];
	time_t status_msg_time;
} state;

#endif // STATE_H
