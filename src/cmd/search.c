#include "prelude.h"
#include "util.h"

void cmd_search(bool forward, wchar_t **args){
	const wchar_t *search = args[1];

	if (!search){
		search = editor_prompt(L"Search text", search, NULL);
		if (!search || wstrlen(search) == 0){
			return;
		}
	}

	bool found = false;
	if (forward){
		for (int i = buffers.curr->cy; i < buffers.curr->num_lines; i++){
			wstring_t *line;
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
			wstring_t *line;
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
