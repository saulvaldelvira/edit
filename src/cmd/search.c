#include "prelude.h"

void cmd_search(bool forward, wchar_t **args){
	static wchar_t *search;

	WString *search_wstr;
	if (!args[1]){
		search_wstr = editor_prompt(L"Search text", search, NULL);
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
