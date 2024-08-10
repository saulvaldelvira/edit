#ifndef STATE_H
#define STATE_H

#include "./lib/GDS/src/Vector.h"
#include <definitions.h>
#include <time.h>
#include <wchar.h>

extern struct state {
	int screen_rows;
	int screen_cols;
	Vector *render;
	wchar_t status_msg[160];
	time_t status_msg_time;
} state;

long get_time_since_start_ms(void);
void editor_on_update(void);
void change_current_buffer_filename(wchar_t *filename);
void NORETURN die(char *msg);
void NORETURN editor_end(void);
void editor_shutdown(void);

#endif // STATE_H
