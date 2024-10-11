#include "prelude.h"
#include "highlight.h"
#include "highlight/color.h"
#include "state.h"
#include "mode.h"

static void default_highlight(void){
	string_t *line;
	for (int i = 0; i < state.screen_rows; i++){
		vector_at(state.render, i, &line);
		int match = str_find_substring(line, "#", 0);
		if (match < 0)
			continue;
		str_insert_cstr(line, Color_IGreen, -1, match);
		str_concat_cstr(line, Color_Reset, -1);
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
