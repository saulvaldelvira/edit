#include "prelude.h"
#include "conf.h"

void cmd_set(wchar_t **args, bool local){
        if (!args[1]) return;
        struct buffer_conf *buf = local ? &buffers.curr->conf : &buffer_conf;

        if (wcscmp(args[1], L"linenumber") == 0){
                buf->line_number = true;
        }
        if (wcscmp(args[1], L"nolinenumber") == 0){
                buf->line_number = false;
        }
        if (wcscmp(args[1], L"tabwidth") == 0){
                if (!args[2])
                        buf->tab_size = buffer_conf.tab_size;
                else
                        swscanf(args[2], L"%ud", &buf->tab_size);
        }
        if (wcscmp(args[1], L"highlight") == 0){
                buf->syntax_highlighting = true;
        }
        if (wcscmp(args[1], L"nohighlight") == 0){
                buf->syntax_highlighting = false;
        }
}
