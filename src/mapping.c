#include "buffer/line.h"
#include "buffer/mode.h"
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
#include "vector.h"

static vector_t *mappings[BUFFER_MODE_LEN] = {0};
static vector_t *mappings_default;
static vector_t *commands;

int __mapping_insert_key(key_ty c) {
        if (is_key_printable(c)) {
                history_entry(change_put_char, c.k, buffers.curr->cx, buffers.curr->cy);
                line_put_char(c.k);
        }
        return SUCCESS;
}

static default_handler_t default_handlers[BUFFER_MODE_LEN] = {0};

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

        vector_append(commands, &cmd);
        size_t len = vector_size(commands);
        return len - 1;
}

static void __register_mapping(buffer_mode_t mode, key_ty key, int confirm_times, int cmd_id) {
        mapping_t m = {
                .cmd_id = cmd_id,
                .key = key,
                .confirm_times = confirm_times,
        };
        vector_t *v = mode == BUFFER_MODE_LEN ? mappings_default : mappings[mode];
        vector_append(v, &m);
}

void register_mapping(buffer_mode_t mode, key_ty key, int confirm_times, command_func_t f, command_arg_t *args) {
        int cmd_id = __register_cmd(f, args);
        __register_mapping(mode, key, confirm_times, cmd_id);
}

static void __register_mapping2(buffer_mode_t mode1, buffer_mode_t mode2, key_ty key, int confirm_times, command_func_t f, command_arg_t *args) {
        int cmd_id = __register_cmd(f, args);
        __register_mapping(mode1, key, confirm_times, cmd_id);
        __register_mapping(mode2, key, confirm_times, cmd_id);
}

void register_default_handler(buffer_mode_t mode, default_handler_t h) {
        default_handlers[mode] = h;
}

#define __cmd_args(...) (command_arg_t[]){ __VA_ARGS__  __VA_OPT__(,) args_end }

#define map_confirm(mode, key, n, f, ...) register_mapping(mode, (key_ty) { .k = key}, n, f, __cmd_args(__VA_ARGS__) )

#define map_confirm_modif(mode, key, n, m, f, ...) register_mapping(mode, (key_ty) { .k = key, .modif = m}, n, f, __cmd_args(__VA_ARGS__) )

#define map_confirm_2(mode1, mode2, key, n, f, ...) __register_mapping2(mode1, mode2, (key_ty) { .k = key}, n, f, __cmd_args(__VA_ARGS__) )

#define map_confirm_modif_2(mode1, mode2, key, n, m, f, ...) __register_mapping2(mode1, mode2, (key_ty) { .k = key, .modif = m}, n, f, __cmd_args(__VA_ARGS__) )

/* #define map_func_confirm(key, n, ...) map_confirm(key, n, command_func_call, __VA_ARGS__) */

#define allmap(key, f, ...) map_confirm(BUFFER_MODE_LEN, key, 0, f, __VA_ARGS__)
#define allmap_modif(key, m, f, ...) map_confirm_modif(BUFFER_MODE_LEN, key, 0, m, f, __VA_ARGS__)
#define allmap_alt(key, f, ...) map_confirm_modif(BUFFER_MODE_LEN, key, 0, KEY_MODIF_ALT, f, __VA_ARGS__)
#define allmap_ctrl(key, f, ...) map_confirm_modif(BUFFER_MODE_LEN, key, 0, KEY_MODIF_CTRL, f, __VA_ARGS__)

#define nmap(key, f, ...) map_confirm(BUFFER_MODE_NORMAL, key, 0, f, __VA_ARGS__)
#define nmap_modif(key, m, f, ...) map_confirm_modif(BUFFER_MODE_NORMAL, key, 0, m, f, __VA_ARGS__)
#define nmap_alt(key, f, ...) map_confirm_modif(BUFFER_MODE_NORMAL, key, 0, KEY_MODIF_ALT, f, __VA_ARGS__)
#define nmap_ctrl(key, f, ...) map_confirm_modif(BUFFER_MODE_NORMAL, key, 0, KEY_MODIF_CTRL, f, __VA_ARGS__)

#define imap(key, f, ...) map_confirm(BUFFER_MODE_INSERT, key, 0, f, __VA_ARGS__)
#define imap_modif(key, m, f, ...) map_confirm_modif(BUFFER_MODE_INSERT, key, 0, m, f, __VA_ARGS__)
#define imap_alt(key, f, ...) map_confirm_modif(BUFFER_MODE_INSERT, key, 0, KEY_MODIF_ALT, f, __VA_ARGS__)
#define imap_ctrl(key, f, ...) map_confirm_modif(BUFFER_MODE_INSERT, key, 0, KEY_MODIF_CTRL, f, __VA_ARGS__)

#define inmap(key, f, ...) map_confirm_2(BUFFER_MODE_NORMAL, BUFFER_MODE_INSERT, key, 0, f, __VA_ARGS__)
#define inmap_modif(key, m, f, ...) map_confirm_modif_2(BUFFER_MODE_NORMAL, BUFFER_MODE_INSERT, key, 0, m, f, __VA_ARGS__)
#define inmap_alt(key, f, ...) map_confirm_modif_2(BUFFER_MODE_NORMAL, BUFFER_MODE_INSERT, key, 0, KEY_MODIF_ALT, f, __VA_ARGS__)
#define inmap_ctrl(key, f, ...) map_confirm_modif_2(BUFFER_MODE_NORMAL, BUFFER_MODE_INSERT, key, 0, KEY_MODIF_CTRL, f, __VA_ARGS__)

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
        editor_log(LOG_ERROR, "%s: Incorrect number of args. Expected %d, found %d", __func__, n, nargs); \
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

__mapping_func_void(map_buffer_drop, {
        buffer_drop(true);
})

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

__mapping_func_void(map_go_insert_on_newline, {
        line_insert_newline_bellow();
        buffer_mode_set(BUFFER_MODE_INSERT);
})

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

__mapping_func1(map_change_mode, int, mode, {
        buffer_mode_set(mode);
})

static void __destroy_command(void *ptr) {
        command_t *cmd = ptr;
        free(cmd->args);
}

void __cleanup_command(void) {
        IGNORE_ON_FAST_CLEANUP(
                vector_free(commands);
                for (int i = 0; i < BUFFER_MODE_LEN; i++) {
                        vector_free(mappings[i]);
                }
        )
}


void init_mapping(void) {
        atexit(__cleanup_command);
        commands = vector_init(sizeof(command_t), compare_equal);
        vector_set_destructor(commands, __destroy_command);
        for (int i = 0; i < BUFFER_MODE_LEN; i++) {
                mappings[i] = vector_init(sizeof(mapping_t), compare_equal);
        }
        mappings_default = vector_init(sizeof(mapping_t), compare_equal);


#define direction_cmd(kc, dir) {\
        int cmd = \
        __register_cmd(map_move_cursor, \
                __cmd_args( \
                        arg_int(CURSOR_DIRECTION_ ## dir),\
                        arg_int(1) \
                ));\
        __register_mapping(BUFFER_MODE_INSERT, (key_ty){ .k = ARROW_ ## dir }, 0, cmd);\
        __register_mapping(BUFFER_MODE_NORMAL, (key_ty){ .k = ARROW_ ## dir }, 0, cmd);\
        __register_mapping(BUFFER_MODE_VISUAL, (key_ty){ .k = ARROW_ ## dir }, 0, cmd);\
        __register_mapping(BUFFER_MODE_INSERT, (key_ty) { .k = kc, .modif = KEY_MODIF_CTRL }, 0, cmd);\
        __register_mapping(BUFFER_MODE_NORMAL, (key_ty) { .k = towlower(kc) }, 0, cmd);\
        __register_mapping(BUFFER_MODE_VISUAL, (key_ty) { .k = towlower(kc) }, 0, cmd);\
}

        direction_cmd('L', RIGHT);
        direction_cmd('H', LEFT);
        direction_cmd('K', UP);
        direction_cmd('J', DOWN);

        inmap(HOME_KEY,
           map_move_cursor,
           arg_int(CURSOR_DIRECTION_START),
           arg_int(1)
        );

        inmap(END_KEY,
           map_move_cursor,
           arg_int(CURSOR_DIRECTION_END),
           arg_int(1)
        );

        inmap(PAGE_UP,
            map_move_cursor,
            arg_int(CURSOR_DIRECTION_PAGE_UP),
            arg_int(1)
        );

        inmap(PAGE_DOWN,
            map_move_cursor,
            arg_int(CURSOR_DIRECTION_PAGE_DOWN),
            arg_int(1)
        );


        imap_ctrl(
            'X',
            map_line_cut,
            arg_bool(true)
        );

        map_confirm_modif_2(
                BUFFER_MODE_INSERT,
                BUFFER_MODE_NORMAL,
                'Q',
                3,
                KEY_MODIF_CTRL,
                map_buffer_drop,
                args_end
        );

        map_confirm_modif(
                BUFFER_MODE_INSERT,
                'O',
                3,
                KEY_MODIF_CTRL,
                map_file_open,
                arg_ptr(NULL)
        );

        map_confirm_modif(
                BUFFER_MODE_INSERT,
                'F',
                3,
                KEY_MODIF_CTRL,
                map_line_format,
                args_end
        );

        inmap_ctrl('N', map_buffer_insert);

        if (conf.history.enabled) {
                imap_ctrl('Z', map_history_undo);
                inmap_ctrl('R', map_history_redo);
                nmap_ctrl('u', map_history_undo);
        }

        inmap_ctrl(
                ARROW_LEFT,
                map_buffer_switch,
                arg_bool(true),
                arg_int(CURSOR_DIRECTION_LEFT)
        );

        inmap_ctrl(
                ARROW_RIGHT,
                map_buffer_switch,
                arg_bool(true),
                arg_int(CURSOR_DIRECTION_RIGHT)
        );

        imap_modif(
                'E',
                KEY_MODIF_CTRL,
                map_cmd_run,
                arg_ptr(NULL)
        );

        nmap(':', map_cmd_run, arg_ptr(NULL));
        nmap(',', map_cmd_run, arg_ptr(NULL));

        imap_modif(
                'S',
                KEY_MODIF_CTRL,
                map_file_save,
                arg_bool(false),
                arg_bool(true)
        );

        imap(
                DEL_KEY,
                map_cursor_delete_char,
                arg_int(CURSOR_DIRECTION_RIGHT)
        );

        nmap(
                DEL_KEY,
                map_move_cursor,
                arg_int(CURSOR_DIRECTION_RIGHT)
        );

        imap(
                BACKSPACE,
                map_cursor_delete_char,
                arg_int(CURSOR_DIRECTION_LEFT)
        );

        nmap(
                BACKSPACE,
                map_move_cursor,
                arg_int(CURSOR_DIRECTION_LEFT),
                arg_int(1)
        );

        nmap(
                '\r',
                map_move_cursor,
                arg_int(CURSOR_DIRECTION_DOWN),
                arg_int(1)
        );

        map_confirm(
                BUFFER_MODE_INSERT,
                F5,
                3,
                map_file_reload
        );

        imap_alt('h', map_help);
        imap_alt('c', map_line_toggle_comment);
        imap_alt(
                's',
                map_cmd_run,
                arg_ptr(L"search")
        );

        nmap(
                '/',
                map_cmd_run,
                arg_ptr(L"search")
        );

        imap_alt(
                's',
                map_cmd_run,
                arg_ptr(L"replace")
       );

        imap_alt(
                'k',
                map_line_cut,
                arg_bool(false)
       );

        imap_alt(
                ARROW_UP,
                map_line_move,
                arg_int(CURSOR_DIRECTION_UP)
        );

        imap_alt(
                ARROW_DOWN,
                map_line_move,
                arg_int(CURSOR_DIRECTION_DOWN)
        );

        imap_alt(
                ARROW_LEFT,
                map_cursor_jump_word,
                arg_int(CURSOR_DIRECTION_LEFT)
        );

        imap_alt(
                ARROW_RIGHT,
                map_cursor_jump_word,
                arg_int(CURSOR_DIRECTION_RIGHT)
        );

        imap_alt(
                BACKSPACE,
                map_cursor_delete_word,
                arg_int(CURSOR_DIRECTION_LEFT)
        );

        imap_alt(
                DEL_KEY,
                map_cursor_delete_word,
                arg_int(CURSOR_DIRECTION_RIGHT)
        );

        nmap(
                'i',
                map_change_mode,
                arg_int(BUFFER_MODE_INSERT)
        );

        nmap(
                'v',
                map_change_mode,
                arg_int(BUFFER_MODE_VISUAL)
        );

        nmap(
                'w',
                map_cursor_jump_word,
                arg_int(CURSOR_DIRECTION_RIGHT)
        );

        nmap(
                'b',
                map_cursor_jump_word,
                arg_int(CURSOR_DIRECTION_LEFT)
        );

        nmap('o', map_go_insert_on_newline);

        allmap_ctrl(
                'C',
                map_change_mode,
                arg_int(BUFFER_MODE_NORMAL)
        );

        allmap(
                ESC,
                map_change_mode,
                arg_int(BUFFER_MODE_NORMAL)
        );

        imap_ctrl(
                'C',
                map_change_mode,
                arg_int(BUFFER_MODE_NORMAL)
        );

        imap(
                ESC,
                map_change_mode,
                arg_int(BUFFER_MODE_NORMAL)
        );

	for (int i = 1; i <= 9; i++){
                imap_alt(
                        (i + '0'),
                        map_buffer_switch,
                        arg_bool(false),
                        arg_int(i)
               );
	}

        register_default_handler(BUFFER_MODE_INSERT, __mapping_insert_key);
}

static int __try_execute_action(vector_t *m, key_ty key) {
        if (key.k == NO_KEY)
                return 1;

        static int confirming_map = -1;
        static int n_confirms = 0;

        for (size_t i = 0; i < vector_size(m); i++) {
                mapping_t map;
                vector_at(m, i, &map);

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
                        editor_set_status_message(L"");

                        command_t cmd;
                        vector_at(commands, map.cmd_id, &cmd);
                        return cmd.func(cmd.nargs, cmd.args);
                }
        }

        n_confirms = 0;

        default_handler_t dh = default_handlers[buffer_mode_get_current()];
        if (dh)
                return dh(key);

        return 0;
}

int try_execute_action(key_ty key) {
        int ret = __try_execute_action(mappings[buffer_mode_get_current()], key);
        if (ret == 0)
                ret = __try_execute_action(mappings_default, key);
        cursor_adjust();
        return ret;
}
