#include <prelude.h>
#include <locale.h>
#include <stdlib.h>
#include "state.h"
#include "definitions.h"
#include "file.h"
#include "init.h"
#include "log.h"
#include "util.h"
#include <lib/json/src/json.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>

struct state state = {
	.status_msg[0] = '\0',
};

static long start_time;

static struct termios original_term;

static void enable_raw_mode(void){
	struct termios term;
	if (tcgetattr(STDIN_FILENO, &term) == -1)
		die("tcgetattr failed");
	original_term = term;
	term.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	term.c_oflag &= ~(OPOST);
	term.c_cflag |= (CS8);
	term.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	term.c_cc[VMIN] = 1;
	term.c_cc[VTIME] = 0;
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term) == -1)
		die("tcsetattr failed");
	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);
}

static INLINE void __enter_alternative_buffer(void) {
	wprintf(L"\x1b[?1049h");
}

static INLINE void __exit_alternative_buffer(void) {
	wprintf(L"\x1b[?1049l");
}

void editor_start_shutdown(void) {
        ONLY_ONCE(
                editor_log(LOG_INFO, "Shutting down editor");
                tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_term); // restore termios
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
	state.render = vector_init(sizeof(WString*), compare_equal);
	vector_set_destructor(state.render, free_wstr);
	for (int i = 0; i < state.screen_rows; i++){
		WString *wstr = wstr_empty();
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
        file_auto_save();
}

INLINE
void change_current_buffer_filename(wchar_t *filename) {
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

