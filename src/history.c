#include "buffer/line.h"
#include "prelude.h"
#include <stdlib.h>
#include "history.h"

#define __hist_get_type_flag(t) _Generic((t), \
                int:  CHANGE_ARG_T_INT, \
                bool: CHANGE_ARG_T_BOOL, \
                char: CHANGE_ARG_T_CHAR, \
                void*: CHANGE_ARG_T_PTR \
                )

#define __hist_get_type_variant(base,t) _Generic((t), \
                int:  base . i, \
                bool: base . b, \
                char: base . c, \
                void*: base . ptr \
                )

#define check_type(t) if (args[_i].type != __hist_get_type_flag(t)) { die("Error!");  };

#define __unwrap_arg3(t,name, ...) \
        t name; \
        check_type(name) \
        name = __hist_get_type_variant(args[_i], name) ; \
        _i++; \
        __VA_OPT__ ( __unwrap_arg(__VA_ARGS__) )

#define __unwrap_arg2(t,name, ...) \
        t name; \
        check_type(name) \
        name = __hist_get_type_variant(args[_i], name) ; \
        _i++; \
        __VA_OPT__ ( __unwrap_arg3(__VA_ARGS__) )

#define __unwrap_arg(t,name, ...) \
        t name; \
        check_type(name) \
        name = __hist_get_type_variant(args[_i], name) ; \
        _i++; \
        __VA_OPT__ ( __unwrap_arg2(__VA_ARGS__) )

#define __check_n_args(n) if (nargs != n) die("Incorrect number of args");

#define __unwrap_args(n, ...) \
        int _i = 0; \
        (void) args; (void) _i; \
        __check_n_args(n) \
        __VA_OPT__ ( __unwrap_arg(__VA_ARGS__) )

#define __args(n, ...) __unwrap_args(n __VA_OPT__( , __VA_ARGS__) )

#define __change_func(name, pre, undo_body, redo_body) \
void __ ## name (change_type_t change_type, int nargs, change_arg_t *args) { \
        pre; \
        if (change_type == CHANGE_UNDO) { \
                undo_body ; \
        } else { \
                redo_body ; \
        } \
} \
change_func_t name = __ ## name;


#define __change_func1(name, t, n, undo_body, redo_body) __change_func(name, __args(1, t, n), undo_body, redo_body)
#define __change_func2(name, t, n, t2, n2, undo_body, redo_body) __change_func(name, __args(2, t, n, t2, n2), undo_body, redo_body)
#define __change_func3(name, t, n, t2, n2, t3, n3, undo_body, redo_body) __change_func(name, __args(3, t, n, t2, n2, t3, n3), undo_body, redo_body)

static change_t __change_register(change_func_t fn, change_arg_t *args) {
        size_t n = 0;
        for (change_arg_t *p = args; p->type != CHANGE_ARG_T_END; p++)
                n++;

        change_t change = {
                .func = fn,
                .nargs = n,
                .args = n > 0 ? malloc(sizeof(change_arg_t) * n) : NULL,
        };
        for (size_t i = 0; i < n; i++) {
                change.args[i] = args[i];
        }

        return change;
}

static bool history_enabled = false;

void history_enable(void) {
        history_enabled = true;
}

void history_push(change_func_t fn, change_arg_t *args) {
        if (history_enabled) {
                change_t c = __change_register(fn, args);
                stack_clear(current_buffer->history.redo);
                stack_push(current_buffer->history.undo, &c);
        }
}

void history_undo(void) {
        change_t c;
        if ( stack_pop(current_buffer->history.undo, &c) == NULL )
                return;
        history_enabled = false;
        c.func(CHANGE_UNDO, c.nargs, c.args);
        stack_push(current_buffer->history.redo, &c);
        history_enabled = true;
}

void history_redo(void) {
        change_t c;
        if ( stack_pop(current_buffer->history.redo, &c) == NULL )
                return;
        history_enabled = false;
        c.func(CHANGE_REDO, c.nargs, c.args);
        stack_push(current_buffer->history.undo, &c);
        history_enabled = true;
}

static void __free_change(void *e) {
        change_t *c = e;
        free(c->args);
}

history_t history_new(void) {
        history_t history;
        history.undo = stack_init(sizeof(change_t), compare_equal);
        stack_set_destructor(history.undo, __free_change);

        history.redo = stack_init(sizeof(change_t), compare_equal);
        stack_set_destructor(history.redo, __free_change);

        return history;
}

void history_free(history_t history) {
        stack_free(history.redo, history.undo);
}

__change_func3(change_put_char, int, key, int, x, int, y,
{
        cursor_goto(x, y);
        line_delete_char(CURSOR_DIRECTION_RIGHT);
}
,
{
        cursor_goto(x, y);
        line_put_char(key);
}
)
