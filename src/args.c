#include "prelude.h"
#include <string.h>
#include <wchar.h>
#include <limits.h>
#include <stdlib.h>
#include "buffer.h"
#include "file.h"
#include "util.h"
#include "args.h"

struct args args = {0};

void args_parse(int argc, char *argv[]) {
	wchar_t filename[NAME_MAX];
	if (argc == 1)
		buffer_insert();
	else
		wprintf(L"\x1b[2J\x1b[H");

	int i;
	for (i = 1; i < argc && argv[i][0] == '-'; i++){
		if (strcmp("--exec", argv[i]) == 0){
			if (i == argc - 1){
				wprintf(L"\x1b[?1049l");
				wprintf(L"Missing argument for \"--exec\" command\n\r");
				exit(1);
			}else{
				args.exec_cmd = argv[++i];
			}
		}else if (strcmp("--", argv[i]) == 0){
			i++;
			break;
		}
	}

	for (; i < argc; i++){
		buffer_insert();
		mbstowcs(filename, argv[i], NAME_MAX);
		filename[NAME_MAX-1] = '\0';
		if (file_open(filename) != 1)
			editor_end();
	}
}
