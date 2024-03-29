#include "input.h"
#include "output.h"
#include "line.h"
#include "file.h"
#include "util.h"
#include "buffer.h"
#include "cmd.h"
#include "cursor.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <wctype.h>
#include <poll.h>

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
		if ((i + '0') == alt_key){
			buffer_switch(i - 1);
			return;
		}
	}
}

int editor_read_key(void){
	static struct pollfd fds[1] = {
		{.fd = STDIN_FILENO, .events = POLLIN}
	};
	if (poll(fds, 1, 0) == 0)
		return NO_KEY;

	wchar_t c = getwchar();
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
#define quit_times_msg(key)						                 \
	do{								                 \
		if (buffers.curr->dirty && quit_times > 0){			         \
			editor_set_status_message(L"WARNING! File has unsaved changes. " \
				                  L"Press %s %d more times to quit.",    \
	                                          key, quit_times);	                 \
			quit_times--;					                 \
			return;						                 \
		}else if (quit_times == 0)				                 \
			editor_set_status_message(L"");			                 \
	}while(0)

	switch (c){
	case '\r':
		line_insert_newline();
		break;
	case CTRL_KEY('q'):
		quit_times_msg("Ctrl + Q");
		buffer_drop();
		break;
	case ARROW_UP:
	case ARROW_DOWN:
	case ARROW_LEFT:
	case ARROW_RIGHT:
	case PAGE_UP:
	case PAGE_DOWN:
	case HOME_KEY:
	case END_KEY:
		cursor_move(c);
		break;
	case CTRL_KEY('k'):
		line_cut(true);
		break;
	case CTRL_KEY('s'):
		file_save(false, true);
		break;
	case CTRL_KEY('o'):
		quit_times_msg("Ctrl + O");
		{
			WString *filename_wstr = editor_prompt(L"Open file", buffers.curr->filename);
			if (filename_wstr && wstr_length(filename_wstr) > 0){
				buffer_clear();
				const wchar_t *filename = wstr_get_buffer(filename_wstr);
				if (file_open(filename) != 1)
					buffer_drop();
			}
			wstr_free(filename_wstr);
		}
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
	case CTRL_KEY('h'):
		line_delete_char_backwards();
		break;
	case F5:
		if (buffers.curr->filename){
			quit_times_msg("F5");
			file_reload();
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

WString* editor_prompt(const wchar_t *prompt, const wchar_t *default_response){
	WString *response = wstr_empty();
	if (default_response)
		wstr_concat_cwstr(response, default_response, FILENAME_MAX);

	size_t x = wstr_length(response);
	size_t base_x = wstrnlen(prompt, -1) + 2;
	int c = 'C';
	while (true){
		size_t offset = 0;
		if (base_x + x + 1 >= (size_t) conf.screen_cols)
			offset = base_x + x + 1 - conf.screen_cols;
		const wchar_t *buf = wstr_get_buffer(response);
		editor_set_status_message(L"%ls: %ls", prompt, &buf[offset]);
		// TODO: check screen size and reisze if needed
		if (c != NO_KEY)
			editor_refresh_screen(true);
		wprintf(L"\x1b[%d;%zuH", conf.screen_rows + 2, base_x + x + 1);
		fflush(stdout);

		c = editor_read_key();

		switch (c)
		{
		case NO_KEY:
		case '\x1b':
		case ARROW_UP:
		case ARROW_DOWN:
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
			editor_set_status_message(L"");
			return response;
		case ARROW_LEFT:
			if (x > 0)
				x--;
			break;
		case ARROW_RIGHT:
			if (x < wstr_length(response))
				x++;
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
			struct pollfd fds[1] = {
				{.fd = STDIN_FILENO, .events = POLLIN}
			};
			poll(fds, 1, 60000);
		}
	}
}

bool editor_ask_confirmation(void){
	WString *response = editor_prompt(L"Are you sure? Y/n", L"Y");
	bool result =
		response != NULL
		&& wstr_length(response) == 1
		&& (wstr_get_at(response, 0) == L'Y'
		    || wstr_get_at(response, 0) == L'y');
	wstr_free(response);
	return result;
}
