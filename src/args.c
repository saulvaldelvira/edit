#include "prelude.h"
#include <string.h>
#include <wchar.h>
#include <limits.h>
#include <stdlib.h>
#include "buffer.h"
#include "file.h"
#include "util.h"
#include "args.h"
#include <cmd.h>

void args_parse(int argc, char *argv[]) {
        struct args {
                char *exec_cmd;
        } args = {0};

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

        if (args.exec_cmd != NULL){
		WString *tmp = wstr_empty();
		wstr_concat_cstr(tmp, args.exec_cmd, -1);
		for (int i = 0; i < buffers.amount; i++){
			buffer_switch(i);
			editor_cmd(wstr_get_buffer(tmp));
			file_save(false, false);
		}
		wstr_free(tmp);
		exit(0);
	}
}
