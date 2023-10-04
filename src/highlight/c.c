#include "../edit.h"
#include "../line.h"
#include "../highlight.h"
#include "color.h"

void highlight_c(void){
	WString *line;
	for (int i = 0; i < conf.screen_rows; i++){
		vector_at(conf.lines_render, i, &line);
		int match = wstr_find_substring(line, L"//", 0);
		if (match >= 0){
			wstr_insert_cwstr(line, Color_IGreen, -1, match);
			wstr_concat_cwstr(line, Color_Reset, -1);
			continue;
		}
		wstr_replace(line, L"#include", Color_Purple L"#include" Color_Reset);
	}
}

