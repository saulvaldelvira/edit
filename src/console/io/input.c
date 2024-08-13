#include "prelude.h"

#include "input.h"
#include "init.h"
#include "linked_list.h"
#include "lib/str/wstr.h"
#include "output.h"
#include "file.h"
#include "util.h"
#include "state.h"
#include "buffer.h"
#include "cmd.h"
#include <console.h>
#include <buffer/line.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <wchar.h>
#include <stdio.h>
#include <wctype.h>
#include "keys.h"

static wstring_t *response = NULL;

static void __cleanup_input(void) {
        CLEANUP_FUNC;
        wstr_free(response);
}

void init_input(void) {
        INIT_FUNC;
        response = wstr_empty();
        atexit(__cleanup_input);
}

static void alt_key_process(void){
	switch (alt_key){
	case L'h':
		editor_help(); break;
	case L'c':
		line_toggle_comment(); break;
	case L's':
		editor_cmd(L"search"); break;
	case L'r':
		editor_cmd(L"replace"); break;
	case L'k':
		line_cut(false); break;
	case 'A': // Alt + UP
		line_move_up(); break;
	case 'B': // Alt + DOWN
		line_move_down(); break;
	case 'C': // Alt + LEFT
		cursor_jump_word(ARROW_RIGHT); break;
	case 'D': // Alt + RIGHT
		cursor_jump_word(ARROW_LEFT); break;
	case 127: // Backspace
		line_delete_word_backwards(); break;
	case DEL_KEY:
		line_delete_word_forward(); break;
	default: break;
	}
	for (int i = 1; i <= 9; i++){
		if ((wchar_t)(i + '0') == alt_key){
			buffer_switch(i - 1);
			return;
		}
	}
}

void editor_process_key_press(int c){
	static int quit_times;
#define confirm_action(key, body)                                                        \
        do{                                                                              \
                if (buffers.curr->dirty && quit_times > 0){                              \
                        editor_set_status_message(L"WARNING! File has unsaved changes. " \
                                                  L"Press %s %d more times to quit.",    \
                                                  key, quit_times);                      \
                        quit_times--;                                                    \
                        return;                                                          \
                }else if (quit_times == 0) {                                             \
                        editor_set_status_message(L"");                                  \
                }                                                                        \
                body                                                                     \
        }while(0)

	switch (c){
	case '\r':
		line_insert_newline();
		break;
	case CTRL_KEY('q'):
                confirm_action("Ctrl + Q", { buffer_drop(); } );
		break;
        case CTRL_KEY('k'):
	case ARROW_UP:
        case CTRL_KEY('j'):
	case ARROW_DOWN:
        case CTRL_KEY('h'):
	case ARROW_LEFT:
        case CTRL_KEY('l'):
	case ARROW_RIGHT:
	case PAGE_UP:
	case PAGE_DOWN:
	case HOME_KEY:
	case END_KEY:
		cursor_move(c);
		break;
	case CTRL_KEY('x'):
		line_cut(true);
		break;
	case CTRL_KEY('s'):
		file_save(false, true);
		break;
	case CTRL_KEY('o'):
		confirm_action("Ctrl + O", { file_open(NULL); });
		break;
	case CTRL_KEY('f'):
		line_format(buffers.curr->cy);
		break;
	case CTRL_KEY('n'):
		buffer_insert();
		break;
	case ALT_KEY:
		alt_key_process();
		break;
	case CTRL_ARROW_LEFT:
		buffer_switch(buffers.curr_index - 1);
		break;
	case CTRL_ARROW_RIGHT:
		buffer_switch(buffers.curr_index + 1);
		break;
	case CTRL_KEY('e'):
		editor_cmd(NULL);
		break;
	case DEL_KEY:
		line_delete_char_forward();
		break;
	case BACKSPACE:
		line_delete_char_backwards();
		break;
	case F5:
		if (buffers.curr->filename){
			confirm_action("F5", { file_reload(); });
		}
		break;
        case NO_KEY:
                return;
	case '\x1b':
		break;
	default:
		if (iswprint(c) || c == L'\t')
			line_put_char(c);
		break;
	}
	quit_times = conf.quit_times;
	cursor_adjust();
}

static inline int __replace_with(wstring_t *wstr, wchar_t *new) {
        wstr_clear(wstr);
        if (new)
                wstr_concat_cwstr(wstr, new, -1);
        return wstr_length(wstr);
}

const wchar_t* editor_prompt(const wchar_t *prompt, const wchar_t *default_response, linked_list_t *history){
        wstr_clear(response);

        list_iterator_t it = {0};
        if (history)
                it = list_iterator_from_back(history);

	if (default_response)
		wstr_concat_cwstr(response, default_response, FILENAME_MAX);

	size_t x = wstr_length(response);
	size_t base_x = wstrlen(prompt) + 2;
	int c = 'C';
        bool end = false;
        enum { UP, DOWN } direction = UP;
	while (!end){
		size_t offset = 0;
		if (base_x + x + 1 >= (size_t) state.screen_cols)
			offset = base_x + x + 1 - state.screen_cols;
		const wchar_t *buf = wstr_get_buffer(response);
		editor_set_status_message(L"%ls: %ls", prompt, &buf[offset]);
		// TODO: check screen size and reisze if needed
		if (c != NO_KEY)
			editor_refresh_screen(true);
		wprintf(L"\x1b[%d;%zuH", state.screen_rows + 2, base_x + x + 1);
		fflush(stdout);

		c = editor_read_key();

		switch (c)
		{
		case NO_KEY:
		case '\x1b':
                        break;
		case DEL_KEY:
		case CTRL_KEY(L'h'):
		case BACKSPACE:
			if (c == DEL_KEY){
				if (x < wstr_length(response))
					wstr_remove_at(response, x);
			}else if (x > 0){
				wstr_remove_at(response, x - 1);
				x--;
			}
			break;
		case CTRL_KEY(L'c'):
			editor_set_status_message(L"");
			wstr_free(response);
			return NULL;
		case '\r':
                        end = true;
                        break;
		case ARROW_LEFT:
			if (x > 0)
				x--;
			break;
		case ARROW_RIGHT:
			if (x < wstr_length(response))
				x++;
			break;
                case ARROW_UP:
                        if (!history) break;
                        wchar_t *prev;
                        if (direction == DOWN) list_it_prev(&it, &prev);
                        direction = UP;
                        if (list_it_prev(&it, &prev) != NULL) {
                                x = __replace_with(response, prev);
                        }
                        break;
                case ARROW_DOWN:
                        if (!history) break;
                        wchar_t *next;
                        if (direction == UP) list_it_next(&it, &next);
                        direction = DOWN;
                        if (list_it_next(&it, &next) != NULL) {
                                x = __replace_with(response, next);
                        }
                        break;
		case HOME_KEY:
			x = 0;
			break;
		case END_KEY:
			x = wstr_length(response);
			break;
		default:
			if (iswprint(c)){
				wstr_insert(response, c, x);
				x++;
			}
			break;
		}
		if (c == NO_KEY){
                        wait_for_input(60000);
		}
	}
        editor_set_status_message(L"");
        if (history) {
                wchar_t *last;
                if (!list_get_back(history, &last)
                        || wstr_cmp_cwstr(response, last) != 0)
                {
                        wchar_t *entry = wstr_cloned_cwstr(response);
                        list_append(history, &entry);
                }
        }
        return wstr_get_buffer(response);
}

bool editor_ask_confirmation(void){
	const wchar_t*response = editor_prompt(L"Are you sure? Y/n", L"Y", NULL);
	bool result =
		response != NULL
		&& wstrlen(response) == 1
		&& (response[0] == L'Y'
		    || response[0] == L'y');
	return result;
}
