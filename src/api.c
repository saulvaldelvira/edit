#include "buffer/line.h"
#include "cmd.h"
#include "file.h"
#include "macros.h"
#include "prelude.h"
#include "console/io.h"
#include "api.h"
#include "console/cursor.h"
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

static vector_t *mappings_vec;
static vector_t *commands_vec;

int __api_insert_key(key_ty c) {
        if (is_key_printable(c))
                line_put_char(c.k);
        return SUCCESS;
}

static default_handler_t default_handler = __api_insert_key;

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
                void*: CMD_ARG_T_PTR, \
                key_ty: CMD_ARG_T_KEY_TY \
                )

#define __get_type_variant(base,t) _Generic((t), \
                int:  base . i, \
                bool: base . b, \
                char: base . c, \
                void*: base . ptr, \
                key_ty: base . key \
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

#define __check_n_args(n) if (nargs != n) return -1;

#define __unwrap_args(n, ...) \
        int _i = 0; \
        (void) args; (void) _i; \
        __check_n_args(n) \
        __VA_OPT__ ( __unwrap_arg(__VA_ARGS__) )

#define __args(n, ...) __unwrap_args(n __VA_OPT__( , __VA_ARGS__) )

#define __api_func(name, pre, body) \
int name (int nargs, command_arg_t *args) { \
        pre; \
        body; \
        return 1; \
}

#define __api_func1(name, t, n, body) __api_func(name, __args(1, t, n), body)
#define __api_func2(name, t, n, t2, n2, body) __api_func(name, __args(2, t, n, t2, n2), body)

#define __api_func_call(name, callee) __api_func(name, __args(0), { callee(); })

#define __api_func_void(name, body) __api_func(name, __args(0), body)

__api_func(
        api_move_cursor,
        __args(2, int, direction, int, n),
{
        while (n-- > 0) {
                cursor_move(direction);
        }
})

__api_func1(
        api_line_cut,
        bool, whole,
        line_cut(whole);
)

__api_func_void(api_line_format, {
        line_format(current_line_row());
})

__api_func_call(api_buffer_drop, buffer_drop)

__api_func1 (
        api_file_open,
        void*, name,

        file_open(name)
)

__api_func2(
        api_buffer_switch,
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

__api_func1(
        api_cmd_run,
        void*, cmd,
        editor_cmd(cmd);
)

__api_func_call(api_buffer_insert, buffer_insert)
__api_func_call(api_help, editor_help)
__api_func_call(api_line_toggle_comment, line_toggle_comment)
__api_func_call(api_file_reload, file_reload)

__api_func1(
        api_line_move,
        int, direction,
{
      return line_move(direction);
})

__api_func1(
        api_cursor_jump_word,
        int, direction,
{
      return cursor_jump_word(direction);
})

__api_func1(
        api_cursor_delete_word,
        int, direction,
{
        return line_delete_word(direction);
})

__api_func1(
        api_cursor_delete_char,
        int, direction,
{
        return line_delete_char(direction);
})

__api_func2(
        api_file_save,
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


void init_command(void) {
        atexit(__cleanup_command);

        commands_vec = vector_init(sizeof(command_t), compare_equal);
        vector_set_destructor(commands_vec, __destroy_command);

        mappings_vec = vector_init(sizeof(mapping_t), compare_equal);


#define direction_cmd(kc, dir) {\
        int cmd = \
        __register_cmd(api_move_cursor, \
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
           api_move_cursor,
           arg_int(CURSOR_DIRECTION_START)
        );

        map(END_KEY,
           api_move_cursor,
           arg_int(CURSOR_DIRECTION_END)
        );

        map(PAGE_UP,
            api_move_cursor,
            arg_int(CURSOR_DIRECTION_PAGE_UP)
        );

        map(PAGE_DOWN,
            api_move_cursor,
            arg_int(CURSOR_DIRECTION_PAGE_DOWN)
        );


        map_ctrl(
            'x',
            api_line_cut,
            arg_bool(true)
        );

        map_confirm_modif(
                'Q',
                3,
                KEY_MODIF_CTRL,
                api_buffer_drop,
                args_end
        );

        map_confirm_modif(
                'O',
                3,
                KEY_MODIF_CTRL,
                api_file_open,
                arg_ptr(NULL)
        );

        map_confirm_modif(
                'F',
                3,
                KEY_MODIF_CTRL,
                api_line_format,
                args_end
        );

        map_ctrl('N', api_buffer_insert);

        map_ctrl(
                ARROW_LEFT,
                api_buffer_switch,
                arg_bool(true),
                arg_int(CURSOR_DIRECTION_LEFT)
        );

        map_ctrl(
                ARROW_RIGHT,
                api_buffer_switch,
                arg_bool(true),
                arg_int(CURSOR_DIRECTION_RIGHT)
        );

        map_modif(
                'E',
                KEY_MODIF_CTRL,
                api_cmd_run,
                arg_ptr(NULL)
        );

        map_modif(
                'S',
                KEY_MODIF_CTRL,
                api_file_save,
                arg_bool(false),
                arg_bool(true)
        );


        map(
                DEL_KEY,
                api_cursor_delete_char,
                arg_int(CURSOR_DIRECTION_RIGHT)
        );

        map(
                BACKSPACE,
                api_cursor_delete_char,
                arg_int(CURSOR_DIRECTION_LEFT)
        );

        map_confirm(
                F5,
                3,
                api_file_reload
        );

        map_alt('h', api_help );
        map_alt('c', api_line_toggle_comment );
        map_alt(
                's',
                api_cmd_run,
                arg_ptr(L"search")
       );

        map_alt(
                's',
                api_cmd_run,
                arg_ptr(L"replace")
       );

        map_alt(
                'k',
                api_line_cut,
                arg_bool(false)
       );

        map_alt(
                ARROW_UP,
                api_line_move,
                arg_int(CURSOR_DIRECTION_UP)
        );

        map_alt(
                ARROW_DOWN,
                api_line_move,
                arg_int(CURSOR_DIRECTION_DOWN)
        );

        map_alt(
                ARROW_LEFT,
                api_cursor_jump_word,
                arg_int(CURSOR_DIRECTION_LEFT)
        );

        map_alt(
                ARROW_RIGHT,
                api_cursor_jump_word,
                arg_int(CURSOR_DIRECTION_RIGHT)
        );

        map_alt(
                BACKSPACE,
                api_cursor_delete_word,
                arg_int(CURSOR_DIRECTION_LEFT)
       );

        map_alt(
                DEL_KEY,
                api_cursor_delete_word,
                arg_int(CURSOR_DIRECTION_RIGHT)
       );

	for (int i = 1; i <= 9; i++){
                map_alt(
                        (i + '0'),
                        api_buffer_switch,
                        arg_bool(false),
                        arg_int(i)
               );
	}
}

__api_func1(
        api_default_handler,
        key_ty, key,
{
        return default_handler(key);
})

command_t __dup_command(command_t *command) {
        command_t cmd = {
                .args = command->nargs > 0 ? malloc(command->nargs * sizeof(command_arg_t)) : NULL,
                .nargs = command->nargs,
                .func = command->func,
        };
        for (int i = 0; i < cmd.nargs; i++)
                cmd.args[i] = command->args[i];
        return cmd;
}

static int __try_get_action(key_ty key, command_t *out) {
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
                                        return 2;
                                }
                        }

                        command_t cmd;
                        vector_at(commands_vec, map.cmd_id, &cmd);
                        *out = cmd;
                        return 1;
                        /* return cmd.func(cmd.nargs, cmd.args); */
                }
        }

        n_confirms = 0;

        static command_arg_t args[1] = {0};
        static command_t def_cmd = {
                .func = api_default_handler,
                .nargs = 1,
                .args =  args,
        };

        if (default_handler) {
                args[0] = (command_arg_t) { .key = key, .type = CMD_ARG_T_KEY_TY };
                *out = def_cmd;
                return 1;
        }
        return 0;
}

int try_execute_action(key_ty key) {
        if (key.k == NO_KEY)
                return 1;
        command_t cmd = {0};
        int ret = __try_get_action(key, &cmd);
        if (ret == 1) {
                ret = run_command(cmd);
                cursor_adjust();
                if (macro_is_recording()) {
                        macro_track_command(__dup_command(&cmd));
                }
        }
        return ret ? SUCCESS : FAILURE;
}

INLINE
int run_command(command_t cmd) {
        return cmd.func(cmd.nargs, cmd.args);
}
