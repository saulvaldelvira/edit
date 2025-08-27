#ifndef __MAPPING_H__
#define __MAPPING_H__

#include "buffer/mode.h"
#include "console/io/keys.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct command_arg {
        enum {
                CMD_ARG_T_INT,
                CMD_ARG_T_BOOL,
                CMD_ARG_T_CHAR,
                CMD_ARG_T_PTR,
                CMD_ARG_T_END,
        } type;
        union {
                int i;
                bool b;
                char c;
                void *ptr;
        };
} command_arg_t;

typedef int (*command_func_t)(int nargs, command_arg_t *args) ;

typedef struct command {
        command_func_t func;
        command_arg_t *args;
        int nargs;
} command_t;

typedef int (*default_handler_t)(key_ty);

void register_default_handler(buffer_mode_t mode, default_handler_t h);

void register_mapping(buffer_mode_t mode, key_ty *keys, size_t keys_len, command_func_t, command_arg_t *args);

int try_execute_action(key_ty key);

void format_keybindings(buffer_mode_t bmode);

#endif /* __MAPPING_H__ */
