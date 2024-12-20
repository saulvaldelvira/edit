#include "cmd.h"
#include "cmd/prelude.h"
#include "init.h"
#include "lib/str/wstr.h"
#include "log.h"
#include "prelude.h"
#include "util.h"

static vector_t *history;

static void __cleanup_cmd(void) {
        CLEANUP_FUNC;

        if (conf.command_history.save_to_file) {
                save_history_to_file("cmd", history);
        }

        IGNORE_ON_FAST_CLEANUP(
                vector_free(history);
        )
}

void init_cmd(void) {
        INIT_FUNC;
        if (conf.command_history.save_to_file) {
                history = load_history_from_file("cmd");
        } else {
                history = vector_init(sizeof(wchar_t*), compare_pointer);
                vector_set_destructor(history, destroy_ptr);
        }
        atexit(__cleanup_cmd);
}

void cmd_search(bool forward, wchar_t **args);
void cmd_replace(wchar_t **args);
void cmd_goto(wchar_t **args);
void cmd_set(wchar_t **args, bool local);

static wchar_t **args;
static wstring_t *cmdstr;

static INLINE void __cmd_end(void) {
        editor_log(LOG_INFO, "CMD: %ls", wstr_get_buffer(cmdstr));
        for (wchar_t **p = args; *p; p++)
                free(*p);
        free(args);
        wstr_free(cmdstr);
}

void editor_cmd(const wchar_t *command){
        if (!command){
                command = editor_prompt(L":", NULL, history);
                if (!command || wstrlen(command) == 0){
                        editor_set_status_message(L"");
                        return;
                }
        }
        cmdstr = wstr_from_cwstr(command, -1);

        args = wstr_split(cmdstr, L" ");
        wchar_t *cmd = args[0];
        if (wcscmp(cmd, L"!quit") == 0){
                if (editor_ask_confirmation()) {
                        __cmd_end();
                        editor_shutdown();
                }
        }
        else if (wcscmp(cmd, L"pwd") == 0){
                editor_set_status_message(L"%s", editor_cwd());
        }
        else if (wcscmp(cmd, L"fwq") == 0){
                bool ask_filename = buffers.curr->filename == NULL;
                for (int i = 0; i < buffers.curr->num_lines; i++)
                        line_format(i);
                int ret = file_save(false, ask_filename);
                if (ret > 0){
                        buffer_drop(true);
                }
        }
        else if (wcscmp(cmd, L"strip") == 0){
                if (!args[1] || wcscmp(args[1], L"line") == 0){
                        line_strip_trailing_spaces(buffers.curr->cy);
                }
                else if (args[1] && wcscmp(args[1], L"buffer") == 0){
                        for (int i = 0; i < buffers.curr->num_lines; i++)
                                line_strip_trailing_spaces(i);
                }else{
                        // TODO: help menu
                        editor_set_status_message(L"Invalid argument for command \"strip\": %ls", args[1]);
                }
        }
        else if (wcscmp(cmd, L"goto") == 0){
                cmd_goto(args);
        }
        else if (wcscmp(cmd, L"search") == 0
                        || wcscmp(cmd, L"search-forward") == 0
                ){
                cmd_search(true, args);
        }
        else if (wcscmp(cmd, L"replace") == 0){
                cmd_replace(args);
        }
        else if (wcscmp(cmd, L"search-backwards") == 0){
                cmd_search(false, args);
        }
        else if (wcscmp(cmd, L"format") == 0){
                if (!args[1] || wcscmp(args[1], L"line") == 0){
                        line_format(buffers.curr->cy);
                }
                else if (args[1] && wcscmp(args[1], L"buffer") == 0){
                        for (int i = 0; i < buffers.curr->num_lines; i++)
                                line_format(i);
                }else{
                        // TODO: help menu
                        editor_set_status_message(L"Invalid argument for command \"format\": %ls", args[1]);
                }
        }
        else if (wcscmp(cmd, L"set") == 0){
                cmd_set(args,false);
        }
        else if (wcscmp(cmd, L"setlocal") == 0){
                cmd_set(args,true);
        }
        else if (wcscmp(cmd, L"help") == 0){
                editor_help();
        }
        else if (wcscmp(cmd, L"log") == 0){
                view_log_buffer();
        }
        else if (wcscmp(cmd, L"undo") == 0){
                history_undo();
        }
        else if (wcscmp(cmd, L"redo") == 0){
                history_undo();
        }

#define cmp(str) else if (wcscmp(cmd, str) == 0)

        cmp(L"w")
                file_save(false, false);

        cmp(L"wa") {
                foreach_buffer(
                    file_save(false, false);
                );
        }

        cmp(L"qa") {
                foreach_buffer(
                        buffer_drop(false);
                );
        }

        cmp(L"qa!") {
                foreach_buffer(
                        buffer_drop(true);
                );
        }

        cmp(L"wq") {
                file_save(false, false);
                buffer_drop(false);
        }

        cmp(L"q")
                buffer_drop(false);
        cmp(L"q!")
                buffer_drop(true);

        else {
                editor_set_status_message(L"Invalid command [%ls]", cmd);
        }
        __cmd_end();
}
