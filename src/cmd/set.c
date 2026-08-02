#include "buffer.h"
#include "console/io/output.h"
#include "prelude.h"
#include "conf.h"

static void do_cmdset_for(struct buffer_conf *buf, wchar_t **args) {
        if (wcscmp(args[1], L"linenumber") == 0){
                buf->line_number = true;
        }
        else if (wcscmp(args[1], L"nolinenumber") == 0){
                buf->line_number = false;
        }
        else if (wcscmp(args[1], L"tabwidth") == 0){
                if (!args[2])
                        buf->tab_size = buffer_conf.tab_size;
                else
                        swscanf(args[2], L"%ud", &buf->tab_size);
        }
        else if (wcscmp(args[1], L"highlight") == 0){
                buf->syntax_highlighting = true;
        }
        else if (wcscmp(args[1], L"nohighlight") == 0){
                buf->syntax_highlighting = false;
        } else {
                editor_set_status_message(L"Unknown 'set' argument: %ls", args[1]);
        }
}

void cmd_set(wchar_t **args, bool local){
        if (!args[1]) return;

        if (local) {
                do_cmdset_for(&buffers.curr->conf, args);
        } else {
                do_cmdset_for(&buffer_conf, args);
                for (int i = 0; i < buffers.amount; i++) {
                        struct buffer *buf = buffer_at(i);
                        do_cmdset_for(&buf->conf, args);
                }
        }
}
