#include "highlight.h"
#include "highlight/color.h"
#include <lib/str/wstr.h>
#include "state.h"
#include "buffer/highlight/mode.h"

static void default_highlight(void){
	wstring_t *line;
	for (int i = 0; i < state.screen_rows; i++){
		vector_at(state.render, i, &line);
		int match = wstr_find_substring(line, L"#", 0);
		if (match < 0)
			continue;
		wstr_insert_cwstr(line, Color_IGreen, -1, match);
		wstr_concat_cwstr(line, Color_Reset, -1);
	}
}

void editor_highlight(void){
	int mode = highligth_mode_current();
	if (mode == NO_MODE) return;

	switch (mode){
	case C_MODE:
		highlight_c(); break;
	case DEFAULT_MODE:
	default:
		default_highlight(); break;
	}
}
