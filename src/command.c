#include "buffer/line.h"
#include "cmd.h"
#include "file.h"
#include "prelude.h"
#include "console/io.h"
#include "command.h"
#include "console/cursor.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

mapping_t mappings[2048];
static int n_mapings = 0;

command_t commands[2048];
static int n_commands = 0;

#define arg_int(val) (command_arg_t) { .i = val, .type = CMD_ARG_T_INT }
#define arg_bool(val) (command_arg_t) { .b = val, .type = CMD_ARG_T_BOOL }
#define arg_ptr(val) (command_arg_t) { .ptr = val, .type = CMD_ARG_T_PTR }
#define arg_func_void_void(val) (command_arg_t) { .func_void_void = val, .type = CMD_ARG_T_FUNC_VOID_VOID }
#define arg_func_void_bool(val) (command_arg_t) { .func_void_bool = val, .type = CMD_ARG_T_FUNC_VOID_BOOL }
#define arg_func_void_int(val) (command_arg_t) { .func_void_int = val, .type = CMD_ARG_T_FUNC_VOID_INT }
#define arg_func_int_ptr(val) (command_arg_t) { .func_int_ptr = ( (int (*) (void*)) val ), .type = CMD_ARG_T_FUNC_INT_PTR }
#define args_end (command_arg_t) { .type = CMD_ARG_T_END }

int command_move_cursor(command_arg_t *args);
int command_buffer_switch(command_arg_t *args);

command_t* make_cmd(command_func_t f, command_arg_t *args) {
        size_t n = 0;
        for (command_arg_t *p = args; p->type != CMD_ARG_T_END; p++)
                n++;

        command_t cmd = {
                .func = f,
                .nargs = n,
                .args = n > 0 ? malloc(sizeof(command_arg_t) * n) : NULL,
        };
        for (size_t i = 0; i < n; i++) {
                cmd.args[i] = args[i];
        }

        commands[n_commands++] = cmd;

        return &commands[n_commands - 1];
}

void register_mapping(int key, int confirm_times, command_t *cmd) {
        mappings[n_mapings++] = (mapping_t) {
                .cmd = cmd,
                .key = key,
                .confirm_times = confirm_times,
        };
}

#define __args(...) (command_arg_t[]){ __VA_ARGS__, args_end}

#define direction_cmd(k, dir) {\
        command_t *cmd = \
        make_cmd(command_move_cursor, \
                __args( \
                        arg_int(DIRECTION_ ## dir),\
                        arg_int(1) \
                ));\
        register_mapping(ARROW_ ## dir, 0, cmd);\
        register_mapping(CTRL_KEY(k), 0, cmd);\
}

#define map_confirm(key, n, f, ...) register_mapping(key, n, make_cmd(f, __args(__VA_ARGS__) ) )

#define map_func_confirm(key, n, ...) map_confirm(key, n, command_func_call, __VA_ARGS__)

#define map(key, f, ...) map_confirm(key, 0, f, __VA_ARGS__)

#define map_func(key, ...) map_func_confirm(key, 0, __VA_ARGS__)

void __cleanup_command(void) {
        for (int i = 0; i < n_commands; i++)
                free(commands[i].args);
}

void init_command(void) {
        atexit(__cleanup_command);

        direction_cmd('l', RIGHT);
        direction_cmd('h', LEFT);
        direction_cmd('k', UP);
        direction_cmd('j', DOWN);

        map(HOME_KEY,
           command_move_cursor,
           arg_int(DIRECTION_START)
        );

        map(END_KEY,
           command_move_cursor,
           arg_int(DIRECTION_END)
        );

        map(PAGE_UP,
            command_func_call,
            arg_func_void_void(cursor_page_up)
        );

        map(PAGE_DOWN,
            command_func_call,
            arg_func_void_void(cursor_page_down)
        );

        map_func(
            CTRL_KEY('x'),
            arg_func_void_bool(line_cut),
            arg_bool(true)
        );

        map_func_confirm(
                CTRL_KEY('q'),
                3,
                arg_func_void_void(buffer_drop)
        );

        map_func_confirm(
                CTRL_KEY('o'),
                3,
                arg_func_int_ptr(file_open),
                arg_ptr(NULL)
        );

        map_func_confirm(
                CTRL_KEY('f'),
                3,
                arg_func_void_void(line_format_current)
        );

        map_func('\r', arg_func_void_void(line_insert_newline));
        map_func(CTRL_KEY('n'), arg_func_void_void(buffer_insert));

        map(
                CTRL_ARROW_LEFT,
                command_buffer_switch,
                arg_int(DIRECTION_LEFT)
        );

        map(
                CTRL_ARROW_RIGHT,
                command_buffer_switch,
                arg_int(DIRECTION_RIGHT)
        );

        map_func(
                CTRL_KEY('e'),
                arg_func_int_constptr(editor_cmd),
                arg_ptr(NULL),
        );
}

int command_move_cursor(command_arg_t *args) {
        int direction = args[0].i;
        int n = args[1].i;
        while (n-- > 0) {
                cursor_move(direction);
        }
        return 1;
}

int try_execute_action(int key) {
        if (key == NO_KEY)
                return 1;

        static mapping_t *confirming_map = NULL;
        static int n_confirms = 0;

        for (int i = 0; i < n_mapings; i++) {
                if (mappings[i].key == key) {
                        mapping_t *curr = &mappings[i];
                        if (curr != confirming_map) {
                                n_confirms = 0;
                        }
                        confirming_map = curr;

                        if (curr->confirm_times > 0 && buffers.curr->dirty) {
                                if (++n_confirms < curr->confirm_times) {
                                        editor_set_status_message(L"WARNING! File has unsaved changes. "
                                                        L"Press %s %d more times to quit.",
                                                        editor_get_key_repr(key),
                                                        curr->confirm_times - n_confirms);
                                        return 1;
                                }
                        }

                        mappings[i].cmd->func(mappings[i].cmd->args);
                        return 1;
                }
        }

        n_confirms = 0;
        return 0;
}

int command_func_call(command_arg_t *args) {
        switch (args[0].type) {
        case CMD_ARG_T_FUNC_VOID_VOID:
                args[0].func_void_void();
                break;
        case CMD_ARG_T_FUNC_VOID_BOOL:
                if (args[1].type != CMD_ARG_T_BOOL)
                        return -1;
                args[0].func_void_bool(args[1].b);
                break;
        default:
                break;
        }
        return SUCCESS;
}

int command_buffer_switch(command_arg_t *args) {
        int dir = args[0].i;
        switch (dir) {
        case DIRECTION_LEFT:
		buffer_switch(buffers.curr_index - 1);
                break;
        case DIRECTION_RIGHT:
		buffer_switch(buffers.curr_index + 1);
                break;
        default:
                break;
        }
        return SUCCESS;
}
