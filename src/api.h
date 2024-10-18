#ifndef __API_H__
#define __API_H__

#include "console/io/keys.h"
#include <stdbool.h>

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

typedef struct mapping {
        key_ty key;
        int confirm_times;
        int cmd_id;
} mapping_t;

typedef int (*default_handler_t)(key_ty);

void register_default_handler(default_handler_t h);

void register_mapping(key_ty key, int confirm_times, command_func_t f, command_arg_t *args);

int try_execute_action(key_ty key);

#endif /* __API_H__ */
