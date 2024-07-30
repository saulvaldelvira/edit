#include "prelude.h"

void cmd_replace(wchar_t **args){
	static wchar_t *text = NULL,*replacement = NULL;

	WString *text_wstr;
	if (!args[1]){
		text_wstr = editor_prompt(L"Replace", text, NULL);
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
		text_wstr = editor_prompt(L"Replace with", replacement, NULL);
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
