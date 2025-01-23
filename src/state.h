#ifndef STATE_H
#define STATE_H

#include <definitions.h>
#include <time.h>
#include <wchar.h>
#include "console/io/keys.h"
#include "vector.h"

extern struct state {
	int screen_rows;
	int screen_cols;
	vector_t *render;
	wchar_t status_msg[512];
	time_t status_msg_time;
} state;

void editor_start(void);
long get_time_since_start_ms(void);
void editor_on_update(void);
void change_current_buffer_filename(wchar_t *filename);
void NORETURN die(const char *msg, const char *fname, int line, const char *func);
void NORETURN editor_shutdown(void);
void NORETURN editor_shutdown_with_error(int code);

void received_key(key_ty c);
void updated_status_line(void);
bool must_render_buffer(void);
bool must_render_stateline(void);

#define die(msg) die(msg, __FILE__, __LINE__, __func__)

#endif // STATE_H
