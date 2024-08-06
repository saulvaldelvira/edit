#include "prelude.h"

#include "input.h"
#include "lib/GDS/src/LinkedList.h"
#include "lib/str/wstr.h"
#include "output.h"
#include "line.h"
#include "file.h"
#include "util.h"
#include "state.h"
#include "buffer.h"
#include "cmd.h"
#include "cursor.h"
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <wchar.h>
#include <stdio.h>
#include <wctype.h>

static wchar_t alt_key;

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

int editor_read_key(void){
	static wchar_t c;
        if (!try_read_char(&c))
                return NO_KEY;
	if (c == L'\x1b') {
		wint_t seq[5];
		if ((seq[0] = getwchar()) == WEOF)
			return '\x1b';
		if (seq[0] == L'[') {
			if ((seq[1] = getwchar()) == WEOF)
				return '\x1b';
			if (iswdigit(seq[1])){
				if ((seq[2] = getwchar()) == WEOF)
					return '\x1b';
				if (seq[2] == '~'){
					switch (seq[1]){
					case '1':
					case '7':
						return HOME_KEY;
					case '3':
						return DEL_KEY;
					case '4':
					case '8':
						return END_KEY;
					case '5': return PAGE_UP;
					case '6': return PAGE_DOWN;

					}

				}
				if ((seq[3] = getwchar()) == WEOF)
					return '\x1b';
				switch (seq[1]){
				case L'1':
					if (seq[2] == ';'){
						if (seq[3] == '5'){
							if ((seq[4] = getwchar()) == WEOF)
								return '\x1b';
							switch (seq[4]){
							case 'C':
								return CTRL_ARROW_RIGHT;
							case 'D':
								return CTRL_ARROW_LEFT;
							default:
								return '\x1b';
							}
						}else if (seq[3] == '3'){
							if ((seq[4] = getwchar()) == WEOF)
								return '\x1b';
							alt_key = seq[4];
							return ALT_KEY;
						}
					}else if (seq[3] == '~'){
						switch (seq[2]){
						case '1': return F1;
						case '2': return F2;
						case '3': return F3;
						case '4': return F4;
						case '5': return F5;
						case '7': return F6;
						case '8': return F7;
						case '9': return F8;
						default: return '\x1b';
						}
					}
					else{
						return '\x1b';
					}
					break;
				case L'2':
					if (seq[3] == '~'){
						switch (seq[2]){
						case '0': return F9;
						case '1': return F10;
						case '3': return F11;
						case '4': return F12;
						default: return '\x1b';
						}
					}else return '\x1b';
				case L'3':
					if (seq[2] == ';'){
						if (seq[3] == '3'){
							if ((seq[4] = getwchar()) == WEOF)
								return '\x1b';
							switch (seq[4]){
							case '~':
								alt_key = DEL_KEY;
								return ALT_KEY;
							default: return '\x1b';
							}
						}else return '\x1b';
					}else return '\x1b';
				default:
					return '\x1b';

				}
			}else{
				switch (seq[1]) {
				case 'A': return ARROW_UP;
				case 'B': return ARROW_DOWN;
				case 'C': return ARROW_RIGHT;
				case 'D': return ARROW_LEFT;
				case 'H': return HOME_KEY;
				case 'F': return END_KEY;
				}
			}
		}else if (seq[0] == 'O'){
			if ((seq[1] = getwchar()) == WEOF)
				return '\x1b';
			switch (seq[1]){
			case 'H': return HOME_KEY;
			case 'F': return END_KEY;
			}
		}else{
			alt_key = seq[0];
			return ALT_KEY;
		}
		return '\x1b';
	} else {
		return c;
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
	case '\x1b':
		break;
	case NO_KEY:
		file_auto_save();
		return;
	default:
		if (iswprint(c) || c == L'\t')
			line_put_char(c);
		break;
	}
	quit_times = conf.quit_times;
	cursor_adjust();
}

static inline int __replace_with(WString *wstr, wchar_t *new) {
        wstr_clear(wstr);
        if (new)
                wstr_concat_cwstr(wstr, new, -1);
        return wstr_length(wstr);
}

WString* editor_prompt(const wchar_t *prompt, const wchar_t *default_response, LinkedList *history){
        LinkedListIterator it = {0};
        if (history)
                it = list_iterator_from_back(history);

	WString *response = wstr_empty();
	if (default_response)
		wstr_concat_cwstr(response, default_response, FILENAME_MAX);

	size_t x = wstr_length(response);
	size_t base_x = wstrnlen(prompt, -1) + 2;
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
        return response;
}

bool editor_ask_confirmation(void){
	WString *response = editor_prompt(L"Are you sure? Y/n", L"Y", NULL);
	bool result =
		response != NULL
		&& wstr_length(response) == 1
		&& (wstr_get_at(response, 0) == L'Y'
		    || wstr_get_at(response, 0) == L'y');
	wstr_free(response);
	return result;
}
