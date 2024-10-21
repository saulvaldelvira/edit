#include "api.h"
#include "prelude.h"
#include "macros.h"
#include "avl_tree.h"
#include "vector.h"
#include "init.h"
#include <stdlib.h>
#include <string.h>

static avl_t *macros_set;

static bool is_first_command;

struct macro {
        wchar_t *name;
        vector_t *actions;
};

struct macro recorder = {0};

static int compare_macro(const void *l, const void *r) {
        const struct macro *lm = l, *rm = r;;
        return wcscmp(lm->name, rm->name);
}

static void free_macro(void *e) {
        struct macro *m = e;
        free(m->name);
        vector_free(m->actions);
}

static void __cleanup_macros(void) {
        IGNORE_ON_FAST_CLEANUP(
                avl_free(macros_set);
        )
}

static void free_command(void *e) {
        command_t *cmd = e;
        free(cmd->args);
}

void init_macros(void) {
        INIT_FUNC;

        atexit(__cleanup_macros);
        macros_set = avl_init(sizeof(struct macro), compare_macro);
        avl_set_destructor(macros_set, free_macro);
}

void macro_start(wchar_t *name) {
        vector_free(recorder.actions);
        free(recorder.name);

        recorder.actions = vector_init(sizeof(command_t), compare_equal);
        vector_set_destructor(recorder.actions, free_command);
        size_t len = wcslen(name);
        recorder.name = malloc(len * sizeof(wchar_t) + 1);
        wcsncpy(recorder.name, name, len);

        is_first_command = true;
}

void macro_end(void) {
        if (recorder.actions) {
                vector_shrink(recorder.actions);
                avl_add(macros_set, &recorder);
                recorder = (struct macro) {0};
        }
}

void macro_play(wchar_t *name) {
        struct macro m;
        if ( avl_get(macros_set, &(struct macro) { .name = name }, &m) != NULL ) {
                for (size_t i = 0; i < vector_size(m.actions); i++) {
                        command_t cmd;
                        vector_at(m.actions, i, &cmd);
                        run_command(cmd);
                }
        }
}

bool macro_is_recording(void) {
        return recorder.actions;
}

void macro_track_command(command_t cmd) {
        if (macro_is_recording() && !is_first_command) {
                vector_append(recorder.actions, &cmd);
        }
        is_first_command = false;
}
