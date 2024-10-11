#include "mode.h"
#include "prelude.h"
#include "buffer.h"
#include <stdlib.h>
#include <string.h>

char * mode_comments[][2] = {
	{"",   NULL},
	{"//", NULL},
	{"#",  NULL},
	{"<!--", "-->"}
};

static char* get_filename_ext(void){
	string_t *tmp_filename_wstr = str_from_cstr(buffers.curr->filename, -1);
	int i = str_find_substring(tmp_filename_wstr, ".", 1);
	int prev_i = -1;
	while (i >= 0){
		prev_i = i;
		i = str_find_substring(tmp_filename_wstr, ".", i + 1);
	}
	if (prev_i < 0){
		str_free(tmp_filename_wstr);
		return NULL;
	}
	char *ext = str_substring(tmp_filename_wstr, prev_i + 1, -1);
	str_free(tmp_filename_wstr);
	return ext;
}

int mode_get_current(void){
	if (!buffers.curr->filename)
		return NO_MODE;
	char *ext = get_filename_ext();
	if (!ext)
		return DEFAULT_MODE;
	int mode;
	if (strcmp(ext, "c") == 0
	    || strcmp(ext, "h") == 0)
	{
		mode = C_MODE;
	}
	else if (strcmp(ext, "html") == 0){
		mode = HTML_MODE;
	}
	else{
		mode = DEFAULT_MODE;
	}
	free(ext);
	return mode;
}
