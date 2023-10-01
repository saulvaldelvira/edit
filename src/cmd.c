#include "cmd.h"
#include "edit.h"
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
		if (y < 0 || y >= conf.num_lines)
			editor_set_status_message(L"Invalid line number %ld", y + 1);
		else
			cursor_goto(conf.cx, y);
	}else{
		if (y < 0 || y >= conf.n_buffers)
			editor_set_status_message(L"Invalid buffer number %ld", y + 1);
		else
			buffer_switch(y);
	}
}

static void cmd_search(bool forward, wchar_t **args){
	// TODO: save last search
	wchar_t *arg = args[1];
	WString *search_wstr = NULL;
	const wchar_t *search;
	if (!arg){
		search_wstr = editor_prompt(L"Search text", NULL);
		if (!search_wstr || wstr_length(search_wstr) == 0){
			wstr_free(search_wstr);
			return;
		}
		search = wstr_get_buffer(search_wstr);
	}else{
		search = arg;
	}

	bool found = false;

	if (forward){
		for (int i = conf.cy; i < conf.num_lines; i++){
			WString *line;
			vector_at(conf.lines, i, &line);
			int x = i == conf.cy ? conf.cx + 1 : 0;
			int substring_index = wstr_find_substring(line, search, x);
			if (substring_index >= 0){
				cursor_goto(substring_index, i);
				found = true;
				break;
			}
		}
	}else{
		for (int i = conf.cy; i >= 0; i--){
			WString *line;
	       	 vector_at(conf.lines, i, &line);

			unsigned int x = i == conf.cy ? (unsigned)conf.cx : wstr_length(line);
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

	wstr_free(search_wstr);
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
	}else if (wcscmp(cmd, L"pwd") == 0){
		editor_set_status_message(L"%s", editor_cwd());
	}else if (wcscmp(cmd, L"wq") == 0){
		bool ask_filename = conf.filename == NULL;
		int ret = file_save(false, ask_filename);
		if (ret > 0){
			buffer_drop();
		}
	}else if (wcscmp(cmd, L"strip") == 0){
		if (!args[1] || wcscmp(args[1], L"line") == 0){
			line_strip_trailing_spaces(conf.cy);
		}
		else if (args[1] && wcscmp(args[1], L"buffer") == 0){
			for (int i = 0; i < conf.num_lines; i++)
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
	else if (wcscmp(cmd, L"search-backwards") == 0){
		cmd_search(false, args);
	}
	else if (wcscmp(cmd, L"format") == 0){
		if (!args[1] || wcscmp(args[1], L"line") == 0){
			line_format(conf.cy);
		}
		else if (args[1] && wcscmp(args[1], L"buffer") == 0){
			for (int i = 0; i < conf.num_lines; i++)
				line_format(i);
		}else{
			// TODO: help menu
			editor_set_status_message(L"Invalid argument for command \"format\": %ls", args[1]);
		}
	}
	// TODO: help command
	else {
		editor_set_status_message(L"Invalid command [%ls]", cmd);
	}
	while (*args)
		free(*args++);
	free(last_cmd);
	last_cmd = wstr_to_cwstr(cmdstr);
	wstr_free(cmdstr);
}
