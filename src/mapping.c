#include "buffer/line.h"
#include "cmd.h"
#include "file.h"
#include "console/io.h"
#include "mapping.h"
#include "console/cursor.h"
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "history.h"
#include "log.h"

static vector_t *mappings_vec;
static vector_t *commands_vec;

int __mapping_insert_key(key_ty c) {
        if (is_key_printable(c)) {
                history_entry(change_put_char, c.k, buffers.curr->cx, buffers.curr->cy);
                line_put_char(c.k);
        }
        return SUCCESS;
}

static default_handler_t default_handler = __mapping_insert_key;

#define arg_int(val) (command_arg_t) { .i = val, .type = CMD_ARG_T_INT }
#define arg_bool(val) (command_arg_t) { .b = val, .type = CMD_ARG_T_BOOL }
#define arg_ptr(val) (command_arg_t) { .ptr = val, .type = CMD_ARG_T_PTR }
#define args_end (command_arg_t) { .type = CMD_ARG_T_END }

static int __register_cmd(command_func_t f, command_arg_t *args) {
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

        vector_append(commands_vec, &cmd);
        size_t len = vector_size(commands_vec);
        return len - 1;
}

static void __register_mapping(key_ty key, int confirm_times, int cmd_id) {
        mapping_t m = {
                .cmd_id = cmd_id,
                .key = key,
                .confirm_times = confirm_times,
        };
        vector_append(mappings_vec, &m);
}

void register_mapping(key_ty key, int confirm_times, command_func_t f, command_arg_t *args) {
        int cmd_id = __register_cmd(f, args);
        __register_mapping(key, confirm_times, cmd_id);
}

void register_default_handler(default_handler_t h) {
        default_handler = h;
}

#define __cmd_args(...) (command_arg_t[]){ __VA_ARGS__  __VA_OPT__(,) args_end }

#define map_confirm(key, n, f, ...) register_mapping((key_ty) { .k = key}, n, f, __cmd_args(__VA_ARGS__) )

#define map_confirm_modif(key, n, m, f, ...) register_mapping((key_ty) { .k = key, .modif = m}, n, f, __cmd_args(__VA_ARGS__) )

/* #define map_func_confirm(key, n, ...) map_confirm(key, n, command_func_call, __VA_ARGS__) */

#define map(key, f, ...) map_confirm(key, 0, f, __VA_ARGS__)

#define map_modif(key, m, f, ...) map_confirm_modif(key, 0, m, f, __VA_ARGS__)
#define map_alt(key, f, ...) map_confirm_modif(key, 0, KEY_MODIF_ALT, f, __VA_ARGS__)
#define map_ctrl(key, f, ...) map_confirm_modif(key, 0, KEY_MODIF_CTRL, f, __VA_ARGS__)

/* #define map_func(key, ...) map_func_confirm(key, 0, __VA_ARGS__) */

#define __get_type_flag(t) _Generic((t), \
                int:  CMD_ARG_T_INT, \
                bool: CMD_ARG_T_BOOL, \
                char: CMD_ARG_T_CHAR, \
                void*: CMD_ARG_T_PTR \
                )

#define __get_type_variant(base,t) _Generic((t), \
                int:  base . i, \
                bool: base . b, \
                char: base . c, \
                void*: base . ptr \
                )

#define check_type(t) if (args[_i].type != __get_type_flag(t)) { die("Error!");  return -1; };

#define __unwrap_arg2(t,name, ...) \
        t name; \
        check_type(name) \
        name = __get_type_variant(args[_i], name) ; \
        _i++; \
        __VA_OPT__ ( __unwrap_arg(__VA_ARGS__) )

#define __unwrap_arg(t,name, ...) \
        t name; \
        check_type(name) \
        name = __get_type_variant(args[_i], name) ; \
        _i++; \
        __VA_OPT__ ( __unwrap_arg2(__VA_ARGS__) )

#define __check_n_args(n) if (nargs != n) {\
        editor_log(LOG_ERROR, "Incorrect number of args: %s", __func__); \
        return -1; }

#define __unwrap_args(n, ...) \
        int _i = 0; \
        (void) args; (void) _i; \
        __check_n_args(n) \
        __VA_OPT__ ( __unwrap_arg(__VA_ARGS__) )

#define __args(n, ...) __unwrap_args(n __VA_OPT__( , __VA_ARGS__) )

#define __mapping_func(name, pre, body) \
int name (int nargs, command_arg_t *args) { \
        pre; \
        body; \
        return 1; \
}

#define __mapping_func1(name, t, n, body) __mapping_func(name, __args(1, t, n), body)
#define __mapping_func2(name, t, n, t2, n2, body) __mapping_func(name, __args(2, t, n, t2, n2), body)

#define __mapping_func_call(name, callee) __mapping_func(name, __args(0), { callee(); })

#define __mapping_func_void(name, body) __mapping_func(name, __args(0), body)

__mapping_func(
        map_move_cursor,
        __args(2, int, direction, int, n),
{
        while (n-- > 0) {
                cursor_move(direction);
        }
})

__mapping_func1(
        map_line_cut,
        bool, whole,
        line_cut(whole);
)

__mapping_func_void(map_line_format, {
        line_format(current_line_row());
})

__mapping_func_call(map_buffer_drop, buffer_drop)

__mapping_func1 (
        map_file_open,
        void*, name,

        file_open(name)
)

__mapping_func2(
        map_buffer_switch,
        bool, relative,
        int, direction,
{
        if (relative) {
                switch (direction) {
                case CURSOR_DIRECTION_LEFT:
                        buffer_switch(buffer_current_index() - 1);
                        break;
                case CURSOR_DIRECTION_RIGHT:
                        buffer_switch(buffer_current_index() + 1);
                        break;
                default:
                        return -1;
                }
        } else {
                buffer_switch(direction);
        }
})

__mapping_func1(
        map_cmd_run,
        void*, cmd,
        editor_cmd(cmd);
)

__mapping_func_call(map_buffer_insert, buffer_insert)
__mapping_func_call(map_help, editor_help)
__mapping_func_call(map_line_toggle_comment, line_toggle_comment)
__mapping_func_call(map_file_reload, file_reload)
__mapping_func_call(map_history_undo, history_undo)
__mapping_func_call(map_history_redo, history_redo)

__mapping_func1(
        map_line_move,
        int, direction,
{
      return line_move(direction);
})

__mapping_func1(
        map_cursor_jump_word,
        int, direction,
{
      return cursor_jump_word(direction);
})

__mapping_func1(
        map_cursor_delete_word,
        int, direction,
{
        return line_delete_word(direction);
})

__mapping_func1(
        map_cursor_delete_char,
        int, direction,
{
        return line_delete_char(direction);
})

__mapping_func2(
        map_file_save,
        bool, only_tmp,
        bool, ask_filename,

        file_save(only_tmp, ask_filename);
)

static void __destroy_command(void *ptr) {
        command_t *cmd = ptr;
        free(cmd->args);
}

void __cleanup_command(void) {
        IGNORE_ON_FAST_CLEANUP(
                vector_free(commands_vec);
                vector_free(mappings_vec);
        )
}


void init_mapping(void) {
        atexit(__cleanup_command);

        commands_vec = vector_init(sizeof(command_t), compare_equal);
        vector_set_destructor(commands_vec, __destroy_command);

        mappings_vec = vector_init(sizeof(mapping_t), compare_equal);


#define direction_cmd(kc, dir) {\
        int cmd = \
        __register_cmd(map_move_cursor, \
                __cmd_args( \
                        arg_int(CURSOR_DIRECTION_ ## dir),\
                        arg_int(1) \
                ));\
        __register_mapping((key_ty){ .k = ARROW_ ## dir }, 0, cmd);\
        __register_mapping((key_ty) { .k = kc, .modif = KEY_MODIF_CTRL }, 0, cmd);\
}

        direction_cmd('L', RIGHT);
        direction_cmd('H', LEFT);
        direction_cmd('K', UP);
        direction_cmd('J', DOWN);

        map(HOME_KEY,
           map_move_cursor,
           arg_int(CURSOR_DIRECTION_START),
           arg_int(1)
        );

        map(END_KEY,
           map_move_cursor,
           arg_int(CURSOR_DIRECTION_END),
           arg_int(1)
        );

        map(PAGE_UP,
            map_move_cursor,
            arg_int(CURSOR_DIRECTION_PAGE_UP)
        );

        map(PAGE_DOWN,
            map_move_cursor,
            arg_int(CURSOR_DIRECTION_PAGE_DOWN)
        );


        map_ctrl(
            'X',
            map_line_cut,
            arg_bool(true)
        );

        map_confirm_modif(
                'Q',
                3,
                KEY_MODIF_CTRL,
                map_buffer_drop,
                args_end
        );

        map_confirm_modif(
                'O',
                3,
                KEY_MODIF_CTRL,
                map_file_open,
                arg_ptr(NULL)
        );

        map_confirm_modif(
                'F',
                3,
                KEY_MODIF_CTRL,
                map_line_format,
                args_end
        );

        map_ctrl('N', map_buffer_insert);

        if (conf.history.enabled) {
                map_ctrl('Z', map_history_undo);
                map_ctrl('R', map_history_redo);
        }

        map_ctrl(
                ARROW_LEFT,
                map_buffer_switch,
                arg_bool(true),
                arg_int(CURSOR_DIRECTION_LEFT)
        );

        map_ctrl(
                ARROW_RIGHT,
                map_buffer_switch,
                arg_bool(true),
                arg_int(CURSOR_DIRECTION_RIGHT)
        );

        map_modif(
                'E',
                KEY_MODIF_CTRL,
                map_cmd_run,
                arg_ptr(NULL)
        );

        map_modif(
                'S',
                KEY_MODIF_CTRL,
                map_file_save,
                arg_bool(false),
                arg_bool(true)
        );


        map(
                DEL_KEY,
                map_cursor_delete_char,
                arg_int(CURSOR_DIRECTION_RIGHT)
        );

        map(
                BACKSPACE,
                map_cursor_delete_char,
                arg_int(CURSOR_DIRECTION_LEFT)
        );

        map_confirm(
                F5,
                3,
                map_file_reload
        );

        map_alt('h', map_help);
        map_alt('c', map_line_toggle_comment);
        map_alt(
                's',
                map_cmd_run,
                arg_ptr(L"search")
       );

        map_alt(
                's',
                map_cmd_run,
                arg_ptr(L"replace")
       );

        map_alt(
                'k',
                map_line_cut,
                arg_bool(false)
       );

        map_alt(
                ARROW_UP,
                map_line_move,
                arg_int(CURSOR_DIRECTION_UP)
        );

        map_alt(
                ARROW_DOWN,
                map_line_move,
                arg_int(CURSOR_DIRECTION_DOWN)
        );

        map_alt(
                ARROW_LEFT,
                map_cursor_jump_word,
                arg_int(CURSOR_DIRECTION_LEFT)
        );

        map_alt(
                ARROW_RIGHT,
                map_cursor_jump_word,
                arg_int(CURSOR_DIRECTION_RIGHT)
        );

        map_alt(
                BACKSPACE,
                map_cursor_delete_word,
                arg_int(CURSOR_DIRECTION_LEFT)
       );

        map_alt(
                DEL_KEY,
                map_cursor_delete_word,
                arg_int(CURSOR_DIRECTION_RIGHT)
       );

	for (int i = 1; i <= 9; i++){
                map_alt(
                        (i + '0'),
                        map_buffer_switch,
                        arg_bool(false),
                        arg_int(i)
               );
	}
}

static int __try_execute_action(key_ty key) {
        if (key.k == NO_KEY)
                return 1;

        static int confirming_map = -1;
        static int n_confirms = 0;

        for (size_t i = 0; i < vector_size(mappings_vec); i++) {
                mapping_t map;
                vector_at(mappings_vec, i, &map);

                if (map.key.k == key.k
                    && map.key.modif == key.modif)
                {
                        if ((int)i != confirming_map) {
                                n_confirms = 0;
                        }
                        confirming_map = i;

                        if (map.confirm_times > 0 && buffers.curr->dirty) {
                                if (++n_confirms < map.confirm_times) {
                                        editor_set_status_message(L"WARNING! File has unsaved changes. "
                                                        L"Press %s %d more times to quit.",
                                                        editor_get_key_repr(key),
                                                        map.confirm_times - n_confirms);
                                        return 1;
                                }
                        }
                        n_confirms = 0;

                        command_t cmd;
                        vector_at(commands_vec, map.cmd_id, &cmd);
                        return cmd.func(cmd.nargs, cmd.args);
                }
        }

        n_confirms = 0;

        if (default_handler)
                return default_handler(key);

        return 0;
}

int try_execute_action(key_ty key) {
        int ret = __try_execute_action(key);
        cursor_adjust();
        return ret;
}
