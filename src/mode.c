#include "mode.h"
#include "edit.h"
#include <stdlib.h>

wchar_t * mode_comments[][2] = {
	{L"",   NULL},
	{L"//", NULL},
	{L"#",  NULL},
	{L"<!--", L"-->"}
};

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

int mode_get_current(void){
	if (!conf.filename)
		return NO_MODE;
	wchar_t *ext = get_filename_ext();
	if (!ext)
		return DEFAULT_MODE;
	int mode;
	if (wcscmp(ext, L"c") == 0
	    || wcscmp(ext, L"h") == 0)
	{
		mode = C_MODE;
	}
	else if (wcscmp(ext, L"html") == 0){
		mode = HTML_MODE;
	}
	else{
		mode = DEFAULT_MODE;
	}
	free(ext);
	return mode;
}
