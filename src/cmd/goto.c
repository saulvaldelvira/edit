#include "prelude.h"

void cmd_goto(wchar_t **args){
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
    			number = editor_prompt(L"Buffer number", NULL, NULL);
		else
			number = editor_prompt(L"Line number", NULL, NULL);
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
