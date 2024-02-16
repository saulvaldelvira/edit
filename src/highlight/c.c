#include "../edit.h"
#include "../highlight.h"
#include "color.h"

void highlight_c(void){
	WString *line;
	for (int i = 0; i < conf.screen_rows; i++){
		vector_at(conf.render, i, &line);
		int match = wstr_find_substring(line, L"//", 0);
		if (match >= 0){
			wstr_insert_cwstr(line, Color_IGreen, -1, match);
			wstr_concat_cwstr(line, Color_Reset, -1);
			continue;
		}

		wstr_replace(line, L"#include ", Color_Purple L"#include " Color_IYellow);
		wstr_replace(line, L"#define ", Color_Purple L"#define " Color_IBlue);
		wstr_replace(line, L"#ifdef ", Color_Purple L"#ifdef " Color_IBlue);
		wstr_replace(line, L"#ifndef ", Color_Purple L"#ifndef " Color_IBlue);
		wstr_replace(line, L"#endif", Color_Purple L"#endif ");

		wstr_replace(line, L"return", Color_Purple L"return" Color_Reset);
		wstr_replace(line, L"if", Color_Purple L"if" Color_Reset);
		wstr_replace(line, L"while", Color_Purple L"while" Color_Reset);
		wstr_replace(line, L"else", Color_Purple L"else" Color_Reset);
		wstr_replace(line, L"switch", Color_Purple L"switch" Color_Reset);
		wstr_replace(line, L"case", Color_Purple L"case" Color_Reset);
		wstr_replace(line, L"default", Color_Purple L"default" Color_Reset);
		wstr_replace(line, L"break", Color_Purple L"break" Color_Reset);
		wstr_replace(line, L"continue", Color_Purple L"continue" Color_Reset);
		wstr_replace(line, L"goto", Color_Purple L"goto" Color_Reset);
		wstr_concat_cwstr(line, Color_Reset, -1);
	}
}

