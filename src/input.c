#include "input.h"
#include "output.h"
#include "line.h"
#include "file.h"
#include "util.h"
#include "buffer.h"
#include "cmd.h"
#include "cursor.h"
#include <unistd.h>
#include <sys/select.h>
#include <stdlib.h>
#include <stdio.h>

// TODO: buffer and do multiple chars at once to optimize
int editor_read_key(void){
	static struct timeval tv = {
		.tv_sec = 0,
		.tv_usec = 50000
	};
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(STDIN_FILENO, &rfds);
	select(STDIN_FILENO + 1, &rfds, NULL, NULL, &tv);
	if (FD_ISSET(STDIN_FILENO, &rfds) == 0)
		return NO_KEY;

	wchar_t c = getwchar();

	if (c == L'\x1b') {
		wint_t seq[5];
		if ((seq[0] = getwchar()) == WEOF)
			return '\x1b';
		if ((seq[1] = getwchar()) == WEOF)
			return '\x1b';
		if (seq[0] == L'[') {
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
			switch (seq[1]){
			case 'H': return HOME_KEY;
			case 'F': return END_KEY;
			}
		}
		return '\x1b';
	} else {

		return c;
	}

}

void editor_process_key_press(int c){
	static int quit_times = QUIT_TIMES;
	#define quit_times_msg(key) \
		do{ \
			if (conf.dirty && quit_times > 0){ \
				editor_set_status_message(L"WARNING! File has unsaved changes. " \
	       					         "Press %s %d more times to quit.", key, quit_times); \
	       			quit_times--; \
	       			return; \
			}else if (quit_times == 0) \
				editor_set_status_message(L""); \
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
		cursor_move(c);
		break;
	case CTRL_KEY('k'):
		line_cut();
		break;
	case CTRL_KEY('s'):
		file_save(false, true);
		break;
	case HOME_KEY:
		conf.cx = 0;
		break;
	case END_KEY:
		conf.cx = current_line_length();
		break;

	case CTRL_KEY('o'):
		quit_times_msg("Ctrl + O");
		{
			WString *filename_wstr = editor_prompt(L"Open file", conf.filename);
			if (filename_wstr && wstr_length(filename_wstr) > 0){
				buffer_clear();
				const wchar_t *filename = wstr_get_buffer(filename_wstr);
				editor_set_status_message(L"Opening file [%ls] ...", filename);
				editor_refresh_screen(true);
				if (file_open(filename) != 1)
					buffer_drop();
			}
			editor_set_status_message(L"");
			wstr_free(filename_wstr);
		}
		break;

	case CTRL_KEY('f'):
		line_format(conf.cy);
		break;
	case CTRL_KEY('n'):
		buffer_insert();
		break;

	case CTRL_ARROW_LEFT:
		buffer_switch(conf.buffer_index - 1);
		break;
	case CTRL_ARROW_RIGHT:
		buffer_switch(conf.buffer_index + 1);
		break;

	case CTRL_KEY('e'):
		editor_cmd(NULL);
		break;

	case CTRL_KEY('y'):
		line_toogle_comment();
		break;

	case BACKSPACE:
	case CTRL_KEY('h'):
	case DEL_KEY:
		if (c == DEL_KEY && cursor_move(ARROW_RIGHT) != 1)
			break;
		line_delete_char();
		break;
	case F5:
		if (conf.filename){
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

	quit_times = QUIT_TIMES;
	cursor_scroll();
}

WString* editor_prompt(const wchar_t *prompt, wchar_t *default_response){
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
			fd_set rfds;
			FD_ZERO(&rfds);
			FD_SET(STDIN_FILENO, &rfds);
			select(STDIN_FILENO + 1, &rfds, NULL, NULL, &(struct timeval){60,0});
		}
	}
}

bool editor_ask_confirmation(void){
	WString *response = editor_prompt(L"Are you sure? Y/n", L"Y");
	bool result = false;
	if (response && wstr_length(response) == 1 && (wstr_get_at(response, 0) == L'Y' || wstr_get_at(response, 0) == L'y'))
		result = true;
	wstr_free(response);
	return result;
}
