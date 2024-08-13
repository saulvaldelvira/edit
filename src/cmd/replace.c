#include "prelude.h"
#include "util.h"

void cmd_replace(wchar_t **args){
	const wchar_t *text = NULL,*replacement = NULL;

	if (!args[1]){
		text = editor_prompt(L"Replace", text, NULL);
	}else{
		text = args[1];
	}

	if (!args[1] || !args[2]){
		replacement = editor_prompt(L"Replace with", replacement, NULL);
	}else{
		replacement = args[2];
	}

        if (!text || !replacement) return;

	for (int i = 0; i < buffers.curr->num_lines; i++){
		wstring_t *line = line_at(i);
		int ret = wstr_replace(line,text,replacement);
		if (ret > 0) buffers.curr->dirty++;
	}
}
