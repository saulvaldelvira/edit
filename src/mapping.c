#include "buffer.h"
#include "buffer/line.h"
#include "buffer/mode.h"
#include "cmd.h"
#include "console/io/keys.h"
#include "console/io/output.h"
#include "fs.h"
#include "console/io.h"
#include "mapping.h"
#include "console/cursor.h"
#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "hash.h"
#include "history.h"
#include "log.h"
#include "hash_map.h"
#include "prefix_tree.h"

static trie_node_t mappings[BUFFER_MODE_LEN] = {0};
static trie_node_t mappings_default;

static hash_map_t *commands_ids;
static vector_t *commands;

int64_t hash_command(const void *cmd_ptr) {
        command_t *cmd = (command_t*)cmd_ptr;

        int64_t hash_func = (uintptr_t)cmd->func;

        int64_t args_hash_acc = 0;

        for (size_t i = 0; i < (size_t)cmd->nargs; i++) {
                command_arg_t arg = cmd->args[i];
                int64_t arg_hash;
                switch (arg.type) {
                        case CMD_ARG_T_INT:
                                arg_hash = hash_int(&arg.i);
                                break;
                        case CMD_ARG_T_CHAR:
                                arg_hash = hash_char(&arg.c);
                                break;

                        case CMD_ARG_T_BOOL:
                                arg_hash = hash_int(&arg.b);
                                break;

                        case CMD_ARG_T_PTR:
                                arg_hash = hash_ptr(&arg.b);
                                break;
                        default:
                                arg_hash = 0;
                                break;
                }
                arg_hash = arg_hash << 1 | arg.type;
                args_hash_acc += i * arg_hash;
        }

        return hash_func + args_hash_acc * 3;
}

int command_eq(const void *cmd1_ptr, const void *cmd2_ptr) {
        command_t *cmd1 = (command_t*)cmd1_ptr;
        command_t *cmd2 = (command_t*)cmd2_ptr;

        if (cmd1->func != cmd2->func)
                return -1;

        if (cmd1->nargs != cmd2->nargs)
                return -1;

        for (int i = 0; i < cmd1->nargs; i++) {
                command_arg_t arg1 = cmd1->args[i];
                command_arg_t arg2 = cmd2->args[i];

                if (arg1.type != arg2.type)
                        return -1;

                switch (arg1.type) {
                        case CMD_ARG_T_INT:
                                if (arg1.i != arg2.i)
                                        return -1;
                                break;
                        case CMD_ARG_T_CHAR:
                                if (arg1.c != arg2.c)
                                        return -1;
                                break;

                        case CMD_ARG_T_BOOL:
                                if (arg1.b != arg2.b)
                                        return -1;
                                break;

                        case CMD_ARG_T_PTR:
                                if (arg1.ptr != arg2.ptr)
                                        return -1;
                                break;
                        default:
                                return -1;
                                break;
                }
        }

        return 0;
}

int __mapping_insert_key(key_ty c) {
        if (c.k != NO_KEY && is_key_printable(c)) {
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
                .args = args,
        };

        int* maybe_id = hashmap_get_ref(commands_ids, &cmd);
        if (maybe_id)
                return *maybe_id;

        cmd.args = n > 0 ? malloc(sizeof(command_arg_t) * n) : NULL;
        for (size_t i = 0; i < n; i++) {
                cmd.args[i] = args[i];
        }

        vector_append(commands, &cmd);
        int id = vector_size(commands) - 1;

        hashmap_put(commands_ids, &cmd, &id);

        return id;
}

void __register_mapping(buffer_mode_t mode, key_ty *keys, size_t keys_len, int cmd_id, char *desc) {
        trie_node_t *trie = mode == BUFFER_MODE_LEN ? &mappings_default : &mappings[mode];
        trie_add(trie, keys, keys_len, (mapping_t) {
                        .cmd_id = cmd_id,
                        .desc = desc
        });
}

void register_mapping(buffer_mode_t mode, key_ty *keys, size_t keys_len, command_func_t f, command_arg_t *args, char *desc) {
        int cmd_id = __register_cmd(f, args);
        __register_mapping(mode, keys, keys_len, cmd_id, desc);
}

void register_default_handler(buffer_mode_t mode, default_handler_t h) {
        default_handlers[mode] = h;
}

#define __cmd_args(...) (command_arg_t[]){ __VA_ARGS__  __VA_OPT__(,) args_end }

#define map(mode, keys, n, msg, f, ...) register_mapping(mode, keys, n, f, __cmd_args(__VA_ARGS__), msg)

#define map1(mode, key, msg, f, ...) map(mode, (key_ty[]) { key }, 1, msg, f, __VA_ARGS__)

#define map_modif(mode, key, m, msg, f, ...) map1(mode, ((key_ty) { .k = key, .modif = m }), msg, f, __VA_ARGS__)

#define map_2(mode1, mode2, key, msg, f, ...) do { \
        register_mapping(mode1, (key_ty[]) { (key_ty) { .k = key} }, 1, f, __cmd_args(__VA_ARGS__), msg) ;\
        register_mapping(mode2, (key_ty[]) { (key_ty) { .k = key} }, 1, f, __cmd_args(__VA_ARGS__), msg) ; } while(0)

#define map_modif_2(mode1, mode2, key, m, msg, f, ...) do { \
        register_mapping(mode1, (key_ty[]) { (key_ty) { .k = key, .modif = m} }, 1, f, __cmd_args(__VA_ARGS__), msg); \
        register_mapping(mode2, (key_ty[]) { (key_ty) { .k = key, .modif = m} }, 1, f, __cmd_args(__VA_ARGS__), msg); } while (0)

/* #define map_func_confirm(key, n, ...) map_confirm(key, n, command_func_call, __VA_ARGS__) */

#define allmap(key, msg, f, ...) map1(BUFFER_MODE_LEN, (key_ty) { .k = key }, msg, f, __VA_ARGS__)
#define allmap_modif(key, m, msg, f, ...) map_modif(BUFFER_MODE_LEN, key, m, msg, f, __VA_ARGS__)
#define allmap_alt(key, msg, f, ...) map_modif(BUFFER_MODE_LEN, key, KEY_MODIF_ALT, msg, f, __VA_ARGS__)
#define allmap_ctrl(key, msg, f, ...) map_modif(BUFFER_MODE_LEN, key, KEY_MODIF_CTRL, msg, f, __VA_ARGS__)

#define nmap(key, msg, f, ...) map1(BUFFER_MODE_NORMAL, (key_ty) { .k = key }, msg, f, __VA_ARGS__)
#define nmap_modif(key, m, msg, f, ...) map_modif(BUFFER_MODE_NORMAL, key, m, msg, f, __VA_ARGS__)
#define nmap_alt(key, msg, f, ...) map_modif(BUFFER_MODE_NORMAL, key, KEY_MODIF_ALT, msg, f, __VA_ARGS__)
#define nmap_ctrl(key, msg, f, ...) map_modif(BUFFER_MODE_NORMAL, key, KEY_MODIF_CTRL, msg, f, __VA_ARGS__)

#define imap(key, msg, f, ...) map1(BUFFER_MODE_INSERT, (key_ty) { .k = key }, msg, f, __VA_ARGS__)
#define imap_modif(key, m, msg, f, ...) map_modif(BUFFER_MODE_INSERT, key, m, msg, f, __VA_ARGS__)
#define imap_alt(key, msg, f, ...) map_modif(BUFFER_MODE_INSERT, key, KEY_MODIF_ALT, msg, f, __VA_ARGS__)
#define imap_ctrl(key, msg, f, ...) map_modif(BUFFER_MODE_INSERT, key, KEY_MODIF_CTRL, msg, f, __VA_ARGS__)

#define nvmap(key, msg, f, ...) map_2(BUFFER_MODE_NORMAL, BUFFER_MODE_VISUAL, key, msg, f, __VA_ARGS__)

#define inmap(key, msg, f, ...) map_2(BUFFER_MODE_NORMAL, BUFFER_MODE_INSERT, key, msg, f, __VA_ARGS__)
#define inmap_modif(key, m, msg, f, ...) map_modif_2(BUFFER_MODE_NORMAL, BUFFER_MODE_INSERT, key, m, msg, f, __VA_ARGS__)
#define inmap_alt(key, desc, f, ...) map_modif_2(BUFFER_MODE_NORMAL, BUFFER_MODE_INSERT, key, KEY_MODIF_ALT, desc, f, __VA_ARGS__)
#define inmap_ctrl(key, desc, f, ...) map_modif_2(BUFFER_MODE_NORMAL, BUFFER_MODE_INSERT, key, KEY_MODIF_CTRL, desc, f, __VA_ARGS__)

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
__mapping_func_call(map_cursor_selection_start, cursor_start_selection)
__mapping_func_call(map_cursor_selection_stop, cursor_stop_selection)

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
                        trie_free(&mappings[i]);
                }
                trie_free(&mappings_default);
                hashmap_free(commands_ids);
        )
}

int64_t hash_key_ty(const void *e) {
        const key_ty *k = e;
        return (int64_t)k->k << 32 | k->modif;
}

int compare_key_ty(const void *e1, const void *e2) {
        const key_ty *k1 = e1;
        const key_ty *k2 = e2;
        if (k1->modif == k2->modif && k1->k && k2->k)
                return 0;
        return 1;
}

void init_mapping(void) {
        atexit(__cleanup_command);

        commands = vector_init(sizeof(command_t), compare_equal);
        vector_set_destructor(commands, __destroy_command);

        commands_ids = hashmap_init(sizeof(command_t), sizeof(int), hash_command, command_eq);

        for (int i = 0; i < BUFFER_MODE_LEN; i++) {
                mappings[i] = trie_new();
        }
        mappings_default = trie_new();


#define direction_cmd(kc, dir, desc) {\
        int cmd = \
        __register_cmd(map_move_cursor, \
                __cmd_args( \
                        arg_int(CURSOR_DIRECTION_ ## dir),\
                        arg_int(1) \
                ));\
        __register_mapping(BUFFER_MODE_INSERT, (key_ty[]){ (key_ty) { .k = ARROW_ ## dir } }, 1, cmd, desc);\
        __register_mapping(BUFFER_MODE_NORMAL, (key_ty[]){ (key_ty) { .k = ARROW_ ## dir } }, 1, cmd, desc);\
        __register_mapping(BUFFER_MODE_VISUAL, (key_ty[]){ (key_ty) { .k = ARROW_ ## dir } }, 1, cmd, desc);\
        __register_mapping(BUFFER_MODE_INSERT, (key_ty[]) {(key_ty) {  .k = kc, .modif = KEY_MODIF_CTRL } }, 1, cmd, desc);\
        __register_mapping(BUFFER_MODE_NORMAL, (key_ty[]) {(key_ty) {  .k = towlower(kc) } }, 1, cmd, desc);\
        __register_mapping(BUFFER_MODE_VISUAL, (key_ty[]) {(key_ty) {  .k = towlower(kc) } }, 1, cmd, desc);\
}

        direction_cmd('L', RIGHT, "Moves the cursor to the right");
        direction_cmd('H', LEFT, "Moves the cursor to the left");
        direction_cmd('K', UP, "Moves the cursor up");
        direction_cmd('J', DOWN, "Moves the cursor down");

        inmap(HOME_KEY,
           "Moves to the start of the line",
           map_move_cursor,
           arg_int(CURSOR_DIRECTION_START),
           arg_int(1)
        );

        inmap(END_KEY,
           "Moves to the end of the line",
           map_move_cursor,
           arg_int(CURSOR_DIRECTION_END),
           arg_int(1)
        );

        inmap(PAGE_UP,
           "Moves a page up",
            map_move_cursor,
            arg_int(CURSOR_DIRECTION_PAGE_UP),
            arg_int(1)
        );

        inmap(PAGE_DOWN,
           "Moves a page down",
            map_move_cursor,
            arg_int(CURSOR_DIRECTION_PAGE_DOWN),
            arg_int(1)
        );


        inmap_ctrl(
            'X',
            "Cuts the current line",
            map_line_cut,
            arg_bool(true)
        );

        nmap_ctrl(
                'J',
                "Moves cursor 4 lines down",
                map_move_cursor,
                arg_int(CURSOR_DIRECTION_DOWN),
                arg_int(4)
        );

        nmap_ctrl(
                'K',
                "Moves cursor 4 lines up",
                map_move_cursor,
                arg_int(CURSOR_DIRECTION_UP),
                arg_int(4)
        );

        inmap_ctrl('N', "Inserts a new buffer", map_buffer_insert);

        if (conf.history.enabled) {
                imap_ctrl('Z', "Undo", map_history_undo);
                inmap_ctrl('R', "Redo", map_history_redo);
                nmap_ctrl('u', "Undo", map_history_undo);
        }

        inmap_ctrl(
                ARROW_LEFT,
                "Switch onw buffer to the left",
                map_buffer_switch,
                arg_bool(true),
                arg_int(CURSOR_DIRECTION_LEFT)
        );

        inmap_ctrl(
                ARROW_RIGHT,
                "Switch onw buffer to the right",
                map_buffer_switch,
                arg_bool(true),
                arg_int(CURSOR_DIRECTION_RIGHT)
        );

        imap_modif(
                'E',
                KEY_MODIF_CTRL,
                "Run a command",
                map_cmd_run,
                arg_ptr(NULL)
        );

        nmap(':', "Open command prompt", map_cmd_run, arg_ptr(NULL));
        nmap(',',"Open command prompt", map_cmd_run, arg_ptr(NULL));

        imap_modif(
                'S',
                KEY_MODIF_CTRL,
                "Save file",
                map_file_save,
                arg_bool(false),
                arg_bool(true)
        );

        imap(
                DEL_KEY,
                "Delete a character",
                map_cursor_delete_char,
                arg_int(CURSOR_DIRECTION_RIGHT)
        );

        nmap(
                DEL_KEY,
                "Move a character to the left",
                map_move_cursor,
                arg_int(CURSOR_DIRECTION_RIGHT)
        );

        imap(
                BACKSPACE,
                "Delete a character",
                map_cursor_delete_char,
                arg_int(CURSOR_DIRECTION_LEFT)
        );

        nmap(
                BACKSPACE,
                "Move a character to the left",
                map_move_cursor,
                arg_int(CURSOR_DIRECTION_LEFT),
                arg_int(1)
        );

        nmap(
                '\r',
                "Move to the next line",
                map_move_cursor,
                arg_int(CURSOR_DIRECTION_DOWN),
                arg_int(1)
        );

        nmap_alt('h', "Display the help menu", map_help);
        inmap_alt('c', "Toggle comment", map_line_toggle_comment);
        inmap_alt(
                's',
                "Search",
                map_cmd_run,
                arg_ptr(L"search")
        );

        nmap(
                '/',
                "Start search",
                map_cmd_run,
                arg_ptr(L"search")
        );

        imap_alt(
                's',
                "Replace",
                map_cmd_run,
                arg_ptr(L"replace")
       );

        inmap_alt(
                'k',
                "Cut the rest of this line",
                map_line_cut,
                arg_bool(false)
       );

        nmap(
                'D',
                "Delete the rest of the line",
                map_line_cut,
                arg_bool(false)
        );

        imap_alt(
                ARROW_UP,
                "Move up",
                map_line_move,
                arg_int(CURSOR_DIRECTION_UP)
        );

        imap_alt(
                ARROW_DOWN,
                "Move down",
                map_line_move,
                arg_int(CURSOR_DIRECTION_DOWN)
        );

        imap_alt(
                ARROW_LEFT,
                "Jump to the next word",
                map_cursor_jump_word,
                arg_int(CURSOR_DIRECTION_LEFT)
        );

        imap_alt(
                ARROW_RIGHT,
                "Jump to the previous word",
                map_cursor_jump_word,
                arg_int(CURSOR_DIRECTION_RIGHT)
        );

        imap_alt(
                BACKSPACE,
                "Delete one word backwards",
                map_cursor_delete_word,
                arg_int(CURSOR_DIRECTION_LEFT)
        );

        imap_alt(
                DEL_KEY,
                "Delete the next word",
                map_cursor_delete_word,
                arg_int(CURSOR_DIRECTION_RIGHT)
        );

        nmap(
                'i',
                "Switch to insert mode",
                map_change_mode,
                arg_int(BUFFER_MODE_INSERT)
        );

        nmap(
                'v',
                "Switch to visual mode",
                map_change_mode,
                arg_int(BUFFER_MODE_VISUAL)
        );

        nmap(
                'w',
                "Move to the next word",
                map_cursor_jump_word,
                arg_int(CURSOR_DIRECTION_RIGHT)
        );

        nmap(
                'b',
                "Move to the previous word",
                map_cursor_jump_word,
                arg_int(CURSOR_DIRECTION_LEFT)
        );

        nmap('o', "Insert a line bellow", map_go_insert_on_newline);

        nvmap('0',
                "Move to the start of the line",
                map_move_cursor,
                arg_int(CURSOR_DIRECTION_START),
                arg_int(1)
              );

        nvmap('$',
                "Move to the end of the line",
                map_move_cursor,
                arg_int(CURSOR_DIRECTION_END),
                arg_int(1)
              );

        allmap_ctrl(
                'C',
                "Change to normal mode",
                map_change_mode,
                arg_int(BUFFER_MODE_NORMAL)
        );

        allmap(
                ESC,
                "Change to normal mode",
                map_change_mode,
                arg_int(BUFFER_MODE_NORMAL)
        );

        imap_ctrl(
                'C',
                "Change to normal mode",
                map_change_mode,
                arg_int(BUFFER_MODE_NORMAL)
        );

        imap(
                ESC,
                "Change to normal mode",
                map_change_mode,
                arg_int(BUFFER_MODE_NORMAL)
        );

	for (int i = 1; i <= 9; i++){
                imap_alt(
                        (i + '0'),
                        "Swicth to the nth buffer",
                        map_buffer_switch,
                        arg_bool(false),
                        arg_int(i)
               );
	}

        register_default_handler(BUFFER_MODE_INSERT, __mapping_insert_key);

        register_mapping(
                BUFFER_MODE_NORMAL,
                (key_ty[])  {
                        KEY('d'),
                        KEY('d'),
                },
                2,
                map_line_cut,
                __cmd_args(
                        arg_bool(true)
                ),
                "Cuts the current line"
        );

#define wind_move(ctrl_val, move_val, dir, desc) \
        register_mapping( \
                BUFFER_MODE_NORMAL, \
                (key_ty[])  { \
                        KEY_CTRL(ctrl_val), \
                        KEY(move_val), \
                }, \
                2, \
                map_buffer_switch, \
                __cmd_args( \
                        arg_bool(true), \
                        arg_int(dir) \
                ), \
                desc\
        ); \
        register_mapping( \
                BUFFER_MODE_NORMAL, \
                (key_ty[])  { \
                        KEY_CTRL(ctrl_val), \
                        KEY_CTRL(toupper(move_val)), \
                }, \
                2, \
                map_buffer_switch, \
                __cmd_args( \
                        arg_bool(true), \
                        arg_int(dir) \
                ), \
                desc\
        ); \

        wind_move('W', 'l', CURSOR_DIRECTION_RIGHT, "Move to the next buffer");
        wind_move('W', 'h', CURSOR_DIRECTION_LEFT, "Move to the previous buffer");
}

int __try_execute_action(buffer_mode_t bmode, key_ty key) {
        static trie_node_t *currents[BUFFER_MODE_LEN + 1] = {0};

        if (key.k == NO_KEY)
                return 1;

        trie_node_t **curr = &currents[bmode];

        if (*curr == NULL) {
                *curr = bmode == BUFFER_MODE_LEN ? &mappings_default : &mappings[bmode];
        }

        *curr = trie_get_next(*curr, key);

        if (*curr == NULL)
                return 0;

        if (curr[0]->mapping.cmd_id >= 0) {
                command_t cmd;
                vector_at(commands, curr[0]->mapping.cmd_id, &cmd);

                *curr = NULL;

                return cmd.func(cmd.nargs, cmd.args);
        }

        return 1;
}

int try_execute_action(key_ty key) {
        int ret = __try_execute_action(buffer_mode_get_current(), key);
        if (ret == 0)
                ret = __try_execute_action(BUFFER_MODE_LEN, key);
        if (ret == 0) {
                default_handler_t dh = default_handlers[buffer_mode_get_current()];
                if (dh) {
                        return dh(key);
                }
        }
        cursor_adjust();
        return ret;
}

static void print_keymap(const key_ty *keys, size_t len, mapping_t mapping) {
        line_put_str("");
        for (size_t i = 0; i < len; i++) {
                key_ty key =  keys[i];
                line_put_str(editor_get_key_repr(key));
        }
        char tmp[1024];
        if (mapping.desc) {
                snprintf(tmp, 1024, "\t\t %s\n", mapping.desc);
        } else {
                snprintf(tmp, 1024, "\t\t (cmd id = %d)\n", mapping.cmd_id);
        }
        line_put_str(tmp);
}

void format_keybindings(void) {
        for (buffer_mode_t bmode = 0; bmode <= BUFFER_MODE_LEN; bmode++) {
                line_put_str("== ");
                const char *name = bmode == BUFFER_MODE_LEN ? "ALL" : buffer_mode_get_string(bmode);
                line_put_str(name);
                line_put_char('\n');
                trie_node_t *trie = bmode == BUFFER_MODE_LEN ? &mappings_default : &mappings[bmode];
                trie_foreach(trie, print_keymap);
        }
        line_put_char('\n');
        line_put_char('\n');
}
