#include <prelude.h>
#include <locale.h>
#include <stdlib.h>
#include "state.h"
#include "console/io/keys.h"
#include "definitions.h"
#include "file.h"
#include "init.h"
#include "log.h"
#include "platform.h"
#include "util.h"
#include <lib/json/src/json.h>
#include <unistd.h>

struct state state = {
	.status_msg[0] = '\0',
};

long start_time;

static INLINE void __enter_alternative_buffer(void) {
	wprintf(L"\x1b[?1049h");
}

static INLINE void __exit_alternative_buffer(void) {
	wprintf(L"\x1b[?1049l");
}

void editor_start_shutdown(void) {
        ONLY_ONCE(
                editor_log(LOG_INFO, "Shutting down editor");
                restore_termios();
                __exit_alternative_buffer();
        )
}


static void __cleanup_state(void) {
        CLEANUP_FUNC;
	vector_free(state.render);
}

void init_state(void) {
        start_time = get_time_millis();

        INIT_FUNC;

	if (get_window_size(&state.screen_rows, &state.screen_cols) == -1)
		die("get_window_size failed");
	state.screen_rows -= BOTTOM_MENU_HEIGHT;
	state.render = vector_init(sizeof(string_t*), compare_equal);
	vector_set_destructor(state.render, free_str);
	for (int i = 0; i < state.screen_rows; i++){
		string_t *wstr = str_empty();
		vector_append(state.render, &wstr);
	}

        enable_raw_mode();

	setlocale(LC_CTYPE, "");
        __enter_alternative_buffer();

        atexit(__cleanup_state);
}

INLINE
long get_time_since_start_ms(void) {
        return get_time_millis() - start_time;
}

void editor_on_update(void) {
        editor_log(LOG_INFO, "Update cycle");
        file_auto_save();
}

INLINE
void change_current_buffer_filename(char *filename) {
        free(buffers.curr->filename);
        buffers.curr->filename = filename;
}

void (die)(const char *msg, const char *fname, int line, const char *func) {
        editor_start_shutdown();
        fprintf(stderr, "ERROR: %s\n"
                        "In %s, line %d (%s)\n",
                        msg, fname, line, func);
        exit(1);
}


void editor_end(void) {
        ONLY_ONCE_AND_WARN ({
                for (int i = 0; i < buffers.amount; i++)
                        buffer_drop();
        });
        exit(0);
}

int last_c = NO_KEY, c = NO_KEY;

INLINE void received_key(int _c) {
        last_c = c;
        c = _c;
}

long last_status_update;

INLINE void updated_status_line(void) {
        last_status_update = get_time_millis();
}

INLINE bool must_render_buffer(void) {
        return c == NO_KEY && last_c != NO_KEY;
}

INLINE bool must_render_stateline(void) {
        return get_time_millis() - last_status_update > 500;
}
