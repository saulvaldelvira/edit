#include "prelude.h"

void cmd_goto(char **args){
	long y;
	const char *arg = args[1];
	bool buffer = false;
	if (arg){
		if (strcmp(arg, "buffer") == 0){
			buffer = true;
			arg = args[2];
		}else if (strcmp(arg, "line") == 0){
			buffer = false;
			arg = args[2];
		}
	}
	if (!arg) {
		if (buffer)
    			arg = editor_prompt("Buffer number", NULL, NULL);
		else
			arg = editor_prompt("Line number", NULL, NULL);
		if (!arg || strlen(arg) == 0){
			return;
		}
	}
        y = atol(arg);
	y--;
	if (!buffer){
		if (y < 0 || y >= buffers.curr->num_lines)
			editor_set_status_message("Invalid line number %ld", y + 1);
		else
			cursor_goto(buffers.curr->cx, y);
	}else{
		if (y < 0 || y >= buffers.amount)
			editor_set_status_message("Invalid buffer number %ld", y + 1);
		else
			buffer_switch(y);
	}
}
