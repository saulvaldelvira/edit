#include <prelude.h>
#include <state.h>
#include "../highlight.h"
#include "color.h"

void highlight_c(void){
	string_t *line;
	for (int i = 0; i < state.screen_rows; i++){
		vector_at(state.render, i, &line);
		int match = str_find_substring(line, "//", 0);
		if (match >= 0){
			str_insert_cstr(line, Color_IGreen, -1, match);
			str_concat_cstr(line, Color_Reset, -1);
			continue;
		}

		str_replace(line, "#include ", Color_Purple "#include " Color_IYellow);
		str_replace(line, "#define ", Color_Purple "#define " Color_IBlue);
		str_replace(line, "#ifdef ", Color_Purple "#ifdef " Color_IBlue);
		str_replace(line, "#ifndef ", Color_Purple "#ifndef " Color_IBlue);
		str_replace(line, "#endif", Color_Purple "#endif ");

		str_replace(line, "return", Color_Purple "return" Color_Reset);
		str_replace(line, "if", Color_Purple "if" Color_Reset);
		str_replace(line, "while", Color_Purple "while" Color_Reset);
		str_replace(line, "else", Color_Purple "else" Color_Reset);
		str_replace(line, "switch", Color_Purple "switch" Color_Reset);
		str_replace(line, "case", Color_Purple "case" Color_Reset);
		str_replace(line, "default", Color_Purple "default" Color_Reset);
		str_replace(line, "break", Color_Purple "break" Color_Reset);
		str_replace(line, "continue", Color_Purple "continue" Color_Reset);
		str_replace(line, "goto", Color_Purple "goto" Color_Reset);
		str_concat_cstr(line, Color_Reset, -1);
	}
}

