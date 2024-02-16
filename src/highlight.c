#include "highlight.h"
#include "highlight/color.h"
#include "edit.h"
#include "mode.h"

static void default_highlight(void){
	WString *line;
	for (int i = 0; i < conf.screen_rows; i++){
		vector_at(conf.render, i, &line);
		int match = wstr_find_substring(line, L"#", 0);
		if (match < 0)
			continue;
		wstr_insert_cwstr(line, Color_IGreen, -1, match);
		wstr_concat_cwstr(line, Color_Reset, -1);
	}
}

void editor_highlight(void){
	int mode = mode_get_current();
	if (mode == NO_MODE) return;

	switch (mode){
	case C_MODE:
		highlight_c(); break;
	case DEFAULT_MODE:
	default:
		default_highlight(); break;
	}
}
