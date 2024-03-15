#include "cmd.h"
#include "input.h"
#include "output.h"
#include "line.h"
#include "util.h"
#include "file.h"
#include "buffer.h"
#include "cursor.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <wchar.h>

static void cmd_goto(wchar_t **args){
	long y;
	wchar_t *arg = args[1];
	bool buffer = false;
	if (arg){
		if (wcscmp(arg, L"buffer") == 0){
			buffer = true;
			arg = args[2];
		}else if (wcscmp(arg, L"line") == 0){
			buffer = false;
			arg = args[2];
		}
	}
	if (!arg) {
		WString *number;
		if (buffer)
    			number = editor_prompt(L"Buffer number", NULL);
		else
			number = editor_prompt(L"Line number", NULL);
		if (!number || wstr_length(number) == 0){
		 	wstr_free(number);
			return;
		}
		y = wcstol(wstr_get_buffer(number), NULL, 10);
		wstr_free(number);
	}else{
		y = wcstol(arg, NULL, 10);
	}
	y--;
	if (!buffer){
		if (y < 0 || y >= buffers.curr->num_lines)
			editor_set_status_message(L"Invalid line number %ld", y + 1);
		else
			cursor_goto(buffers.curr->cx, y);
	}else{
		if (y < 0 || y >= buffers.amount)
			editor_set_status_message(L"Invalid buffer number %ld", y + 1);
		else
			buffer_switch(y);
	}
}

static void cmd_search(bool forward, wchar_t **args){
	static wchar_t *search;

	WString *search_wstr;
	if (!args[1]){
		search_wstr = editor_prompt(L"Search text", search);
		if (!search_wstr || wstr_length(search_wstr) == 0){
			wstr_free(search_wstr);
			return;
		}
	}else{
		search_wstr = wstr_from_cwstr(args[1], -1);
	}
	free(search);
	search = wstr_to_cwstr(search_wstr);
	wstr_free(search_wstr);

	bool found = false;
	if (forward){
		for (int i = buffers.curr->cy; i < buffers.curr->num_lines; i++){
			WString *line;
			vector_at(buffers.curr->lines, i, &line);
			int x = i == buffers.curr->cy ? buffers.curr->cx + 1 : 0;
			int substring_index = wstr_find_substring(line, search, x);
			if (substring_index >= 0){
				cursor_goto(substring_index, i);
				found = true;
				break;
			}
		}
	}else{
		for (int i = buffers.curr->cy; i >= 0; i--){
			WString *line;
			vector_at(buffers.curr->lines, i, &line);

			unsigned int x = i == buffers.curr->cy ? (unsigned)buffers.curr->cx : wstr_length(line);
			int substring_index = -1;
			for (unsigned int i = 0; i < x; i++){
				int tmp = wstr_find_substring(line, search, i);
				if (tmp >= 0)
					substring_index = tmp;
			}

			if (substring_index >= 0 && substring_index + wstrnlen(search, -1) < x){
				cursor_goto(substring_index, i);
				found = true;
				break;
			}
		}
	}
	if (!found)
		editor_set_status_message(L"String [%ls] not found", search);
}

static void cmd_replace(wchar_t **args){
	static wchar_t *text = NULL,*replacement = NULL;

	WString *text_wstr;
	if (!args[1]){
		text_wstr = editor_prompt(L"Replace", text);
		if (!text_wstr || wstr_length(text_wstr) == 0){
			wstr_free(text_wstr);
			return;
		}
	}else{
		text_wstr = wstr_from_cwstr(args[1], -1);
	}

	free(text);
	text = wstr_to_cwstr(text_wstr);
	wstr_clear(text_wstr);

	if (!args[1] || !args[2]){
		text_wstr = editor_prompt(L"Replace with", replacement);
		if (!text_wstr){
			wstr_free(text_wstr);
			return;
		}
	}else{
		text_wstr = wstr_from_cwstr(args[2], -1);
	}

	free(replacement);
	replacement = wstr_to_cwstr(text_wstr);
	wstr_free(text_wstr);

	for (int i = 0; i < buffers.curr->num_lines; i++){
		WString *line = line_at(i);
		int ret = wstr_replace(line,text,replacement);
		if (ret > 0) buffers.curr->dirty++;
	}
}

void cmd_set(wchar_t **args, bool local){
        if (!args[1]) return;
        struct buffer *buf = local ? buffers.curr : &buffers.default_buffer;

        if (wcscmp(args[1], L"linenumber") == 0){
                buf->line_number = true;
        }
        if (wcscmp(args[1], L"nolinenumber") == 0){
                buf->line_number = false;
        }
        if (wcscmp(args[1], L"tabwidth") == 0){
                if (!args[2])
                        buf->tab_size = buffers.default_buffer.tab_size;
                else
                        swscanf(args[2], L"%ud", &buf->tab_size);
        }
        if (wcscmp(args[1], L"highlight") == 0){
                buf->syntax_highlighting = true;
        }
        if (wcscmp(args[1], L"nohighlight") == 0){
                buf->syntax_highlighting = false;
        }
}

void editor_cmd(const wchar_t *command){
        static wchar_t *last_cmd = NULL;
        WString *cmdstr = NULL;
        if (!command){
                cmdstr = editor_prompt(L"Execute command", last_cmd);
                if (!cmdstr || wstr_length(cmdstr) == 0){
                        editor_set_status_message(L"");
                        wstr_free(cmdstr);
                        return;
                }
        }else {
                cmdstr = wstr_from_cwstr(command, -1);
        }

        wchar_t **args = wstr_split(cmdstr, L" ");
        wchar_t *cmd = args[0];
        if (wcscmp(cmd, L"!quit") == 0){
                if (editor_ask_confirmation())
                        editor_end();
        }
        else if (wcscmp(cmd, L"pwd") == 0){
                editor_set_status_message(L"%s", editor_cwd());
        }
        else if (wcscmp(cmd, L"wq") == 0){
                bool ask_filename = buffers.curr->filename == NULL;
                int ret = file_save(false, ask_filename);
                if (ret > 0){
                        buffer_drop();
                }
        }
        else if (wcscmp(cmd, L"fwq") == 0){
                bool ask_filename = buffers.curr->filename == NULL;
                for (int i = 0; i < buffers.curr->num_lines; i++)
                        line_format(i);
                int ret = file_save(false, ask_filename);
                if (ret > 0){
                        buffer_drop();
                }
        }
        else if (wcscmp(cmd, L"strip") == 0){
                if (!args[1] || wcscmp(args[1], L"line") == 0){
                        line_strip_trailing_spaces(buffers.curr->cy);
                }
                else if (args[1] && wcscmp(args[1], L"buffer") == 0){
                        for (int i = 0; i < buffers.curr->num_lines; i++)
                                line_strip_trailing_spaces(i);
                }else{
                        // TODO: help menu
                        editor_set_status_message(L"Invalid argument for command \"strip\": %ls", args[1]);
                }
        }
        else if (wcscmp(cmd, L"goto") == 0){
                cmd_goto(args);
        }
        else if (wcscmp(cmd, L"search") == 0
                        || wcscmp(cmd, L"search-forward") == 0
                ){
                cmd_search(true, args);
        }
        else if (wcscmp(cmd, L"replace") == 0){
                cmd_replace(args);
        }
        else if (wcscmp(cmd, L"search-backwards") == 0){
                cmd_search(false, args);
        }
        else if (wcscmp(cmd, L"format") == 0){
                if (!args[1] || wcscmp(args[1], L"line") == 0){
                        line_format(buffers.curr->cy);
                }
                else if (args[1] && wcscmp(args[1], L"buffer") == 0){
                        for (int i = 0; i < buffers.curr->num_lines; i++)
                                line_format(i);
                }else{
                        // TODO: help menu
                        editor_set_status_message(L"Invalid argument for command \"format\": %ls", args[1]);
                }
        }
        else if (wcscmp(cmd, L"set") == 0){
                cmd_set(args,false);
        }
        else if (wcscmp(cmd, L"setlocal") == 0){
                cmd_set(args,true);
        }
        else if (wcscmp(cmd, L"help") == 0){
                editor_help();
        }
        else {
                editor_set_status_message(L"Invalid command [%ls]", cmd);
        }
        for (wchar_t **p = args; *p; p++)
                free(*p);
        free(args);
        free(last_cmd);
        last_cmd = wstr_to_cwstr(cmdstr);
        wstr_free(cmdstr);
}
