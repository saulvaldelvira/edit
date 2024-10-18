#ifndef __COMMAND_H__
#define __COMMAND_H__

#include <stdbool.h>
typedef struct command_arg {
        enum {
                CMD_ARG_T_INT,
                CMD_ARG_T_BOOL,
                CMD_ARG_T_CHAR,
                CMD_ARG_T_PTR,
                CMD_ARG_T_END,
                CMD_ARG_T_FUNC_VOID_VOID,
                CMD_ARG_T_FUNC_VOID_BOOL,
                CMD_ARG_T_FUNC_VOID_INT,
                CMD_ARG_T_FUNC_INT_PTR,
                CMD_ARG_T_FUNC_INT_CONSTPTR,
        } type;
        union {
                int i;
                bool b;
                char c;
                void *ptr;
                void (*func_void_void) (void);
                void (*func_void_bool) (bool);
                void (*func_void_int) (int);
                int  (*func_int_ptr) (void*);
                int  (*func_int_constptr) (const void*);
        };
} command_arg_t;

typedef int (*command_func_t)(command_arg_t args[]) ;

typedef struct command {
        command_func_t func;
        command_arg_t *args;
        int nargs;
} command_t;

typedef struct mapping {
        int key;
        int confirm_times;
        command_t *cmd;
} mapping_t;

void register_mapping(int key, int confirm_times, command_t *cmd);

int try_execute_action(int key);

int command_func_call(command_arg_t *args);

#endif /* __COMMAND_H__ */
