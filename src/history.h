#ifndef __HISTORY_H__
#define __HISTORY_H__

#include "stack.h"
typedef enum change_type {
        CHANGE_UNDO,
        CHANGE_REDO,
} change_type_t;

typedef struct change_arg {
        enum {
                CHANGE_ARG_T_INT,
                CHANGE_ARG_T_BOOL,
                CHANGE_ARG_T_CHAR,
                CHANGE_ARG_T_PTR,
                CHANGE_ARG_T_END,
        } type;
        union {
                int i;
                bool b;
                char c;
                void *ptr;
        };
} change_arg_t;

typedef void (*change_func_t)(change_type_t change_type, int nargs, change_arg_t *args) ;

typedef struct change {
        change_func_t func;
        change_arg_t *args;
        int nargs;
} change_t;

typedef struct history {
	stack_t *undo;
	stack_t *redo;
} history_t;

history_t history_new(void);
void history_free(history_t history);

void history_clear(history_t history);
void history_push(change_func_t fn, change_arg_t *args);
void history_undo(void);
void history_redo(void);

static inline change_arg_t history_arg_int(int val) { return (change_arg_t) { .i = val, .type = CHANGE_ARG_T_INT }; }
static inline change_arg_t history_arg_bool(bool val) { return (change_arg_t) { .b = val, .type = CHANGE_ARG_T_BOOL }; }
static inline change_arg_t history_arg_ptr(void *val) { return (change_arg_t) { .ptr = val, .type = CHANGE_ARG_T_PTR }; }
static inline change_arg_t history_arg_char(char val) { return (change_arg_t) { .c = val, .type = CHANGE_ARG_T_CHAR }; }
#define history_args_end (change_arg_t) { .type = CHANGE_ARG_T_END }

#define arg(val) _Generic((val), \
        int:  history_arg_int, \
        bool: history_arg_bool, \
        char: history_arg_char, \
        void*: history_arg_ptr \
)(val)

#define history_entry(fn, ...) history_push(fn, __change_args(__VA_ARGS__))

#define __change_args_inner(c, ...) arg(c) __VA_OPT__(, __change_args_inner2(__VA_ARGS__))
#define __change_args_inner2(c, ...) arg(c) __VA_OPT__(, __change_args_inner3(__VA_ARGS__))
#define __change_args_inner3(c, ...) arg(c) __VA_OPT__(, __change_args_inner(__VA_ARGS__))

#define __change_args(...) (change_arg_t[]){  __VA_OPT__( __change_args_inner(__VA_ARGS__) ,) history_args_end }


#define changes(...) __VA_OPT__( __change(__VA_ARGS__) )

#define __change(c, ...) extern change_func_t c; __VA_OPT__ ( __change2(__VA_ARGS__) )
#define __change2(c, ...) extern change_func_t c; __VA_OPT__ ( __change(__VA_ARGS__) )

changes (
        change_put_char
)

#undef __change
#undef __change2
#undef changes

#endif /* __HISTORY_H__ */
