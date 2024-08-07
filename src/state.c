#include <prelude.h>
#include <stdlib.h>
#include "state.h"
#include "util.h"
#include <lib/json/src/json.h>
#include <time.h>

struct state state = {
	.status_msg[0] = '\0',
};

static long start_time;

static void __cleanup_state(void) {
	vector_free(state.render);
}

void init_state(void) {
	if (get_window_size(&state.screen_rows, &state.screen_cols) == -1)
		die("get_window_size failed");
	state.screen_rows -= BOTTOM_MENU_HEIGHT;
	state.render = vector_init(sizeof(WString*), compare_equal);
	vector_set_destructor(state.render, free_wstr);
	for (int i = 0; i < state.screen_rows; i++){
		WString *wstr = wstr_empty();
		vector_append(state.render, &wstr);
	}
        start_time = get_time_millis();

        atexit(__cleanup_state);
}

long get_time_since_start_ms(void) {
        return get_time_millis() - start_time;
}
