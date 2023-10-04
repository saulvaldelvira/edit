#include "highlight.h"
#include "highlight/color.h"
#include "edit.h"
#include <stdlib.h>

static wchar_t* get_filename_ext(void){
	WString *tmp_filename_wstr = wstr_from_cwstr(conf.filename, -1);
	int i = wstr_find_substring(tmp_filename_wstr, L".", 1);
	int prev_i = -1;
	while (i >= 0){
		prev_i = i;
		i = wstr_find_substring(tmp_filename_wstr, L".", i + 1);
	}
	if (prev_i < 0){
		wstr_free(tmp_filename_wstr);
		return NULL;
	}
	wchar_t *ext = wstr_substring(tmp_filename_wstr, prev_i + 1, -1);
	wstr_free(tmp_filename_wstr);
	return ext;
}

static void default_highlight(void){
	WString *line;
	for (int i = 0; i < conf.screen_rows; i++){
		vector_at(conf.lines_render, i, &line);
		int match = wstr_find_substring(line, L"#", 0);
		if (match < 0)
			continue;
		wstr_insert_cwstr(line, Color_IGreen, -1, match);
		wstr_concat_cwstr(line, Color_Reset, -1);
	}
}

void editor_highlight(void){
	if (!conf.filename)
		return;
	wchar_t *ext = get_filename_ext();
	if (!ext){
		default_highlight();
		return;
	}

	if (wcscmp(ext, L"c") == 0
	 || wcscmp(ext, L"h") == 0)
	{
		highlight_c();
	}else{
		default_highlight();
	}

	free(ext);
}
