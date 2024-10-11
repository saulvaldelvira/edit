#include "cmd/prelude.h"
#include "cmd.h"
#include "init.h"
#include "log.h"
#include "util.h"

static linked_list_t *history;

static void __cleanup_cmd(void) {
        CLEANUP_FUNC;
        list_free(history);
}

void init_cmd(void) {
        INIT_FUNC;
        history = list_init(sizeof(wchar_t*), compare_pointer);
        list_set_destructor(history, destroy_ptr);
        atexit(__cleanup_cmd);
}

void cmd_search(bool forward, char **args);
void cmd_replace(char **args);
void cmd_goto(char **args);
void cmd_set(char **args, bool local);


static char **args;
static string_t *cmdstr;

static INLINE void __cmd_end(void) {
        editor_log(LOG_INFO, "CMD: %ls", str_get_buffer(cmdstr));
        for (char **p = args; *p; p++)
                free(*p);
        free(args);
        str_free(cmdstr);
}

void editor_cmd(const char *command){
        if (!command){
                command = editor_prompt("Execute command", NULL, history);
                if (!command || strlen(command) == 0){
                        editor_set_status_message("");
                        str_free(cmdstr);
                        return;
                }
        }
        cmdstr = str_from_cstr(command, -1);

        args = str_split(cmdstr, " ");
        char *cmd = args[0];
        if (strcmp(cmd, "!quit") == 0){
                if (editor_ask_confirmation()) {
                        __cmd_end();
                        editor_end();
                }
        }
        else if (strcmp(cmd, "pwd") == 0){
                editor_set_status_message("%s", editor_cwd());
        }
        else if (strcmp(cmd, "wq") == 0){
                bool ask_filename = buffers.curr->filename == NULL;
                int ret = file_save(false, ask_filename);
                if (ret > 0){
                        buffer_drop();
                }
        }
        else if (strcmp(cmd, "fwq") == 0){
                bool ask_filename = buffers.curr->filename == NULL;
                for (int i = 0; i < buffers.curr->num_lines; i++)
                        line_format(i);
                int ret = file_save(false, ask_filename);
                if (ret > 0){
                        buffer_drop();
                }
        }
        else if (strcmp(cmd, "strip") == 0){
                if (!args[1] || strcmp(args[1], "line") == 0){
                        line_strip_trailing_spaces(buffers.curr->cy);
                }
                else if (args[1] && strcmp(args[1], "buffer") == 0){
                        for (int i = 0; i < buffers.curr->num_lines; i++)
                                line_strip_trailing_spaces(i);
                }else{
                        // TODO: help menu
                        editor_set_status_message("Invalid argument for command \"strip\": %ls", args[1]);
                }
        }
        else if (strcmp(cmd, "goto") == 0){
                cmd_goto(args);
        }
        else if (strcmp(cmd, "search") == 0
                        || strcmp(cmd, "search-forward") == 0
                ){
                cmd_search(true, args);
        }
        else if (strcmp(cmd, "replace") == 0){
                cmd_replace(args);
        }
        else if (strcmp(cmd, "search-backwards") == 0){
                cmd_search(false, args);
        }
        else if (strcmp(cmd, "format") == 0){
                if (!args[1] || strcmp(args[1], "line") == 0){
                        line_format(buffers.curr->cy);
                }
                else if (args[1] && strcmp(args[1], "buffer") == 0){
                        for (int i = 0; i < buffers.curr->num_lines; i++)
                                line_format(i);
                }else{
                        // TODO: help menu
                        editor_set_status_message("Invalid argument for command \"format\": %ls", args[1]);
                }
        }
        else if (strcmp(cmd, "set") == 0){
                cmd_set(args,false);
        }
        else if (strcmp(cmd, "setlocal") == 0){
                cmd_set(args,true);
        }
        else if (strcmp(cmd, "help") == 0){
                editor_help();
        }
        else if (strcmp(cmd, "log") == 0){
                view_log_buffer();
        }
        else {
                editor_set_status_message("Invalid command [%ls]", cmd);
        }
        __cmd_end();
}
