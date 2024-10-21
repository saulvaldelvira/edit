#include <prelude.h>
#include "state.h"
#include <stdlib.h>
#include <init.h>

void init_state(void);
void init_buffer(void);
void init_cmd(void);
void init_file(void);
void init_io(void);
void init_log(void);
void init_command(void);
void init_macros(void);

void init(void) {
        init_log();
        init_state();

        init_io();
        init_buffer();
        init_cmd();
        init_file();
        init_command();
        init_macros();

        /* The atexit functions are executed in
         * reverse order. So if we want this to
         * be called firs, we need to register it last. */
        atexit(editor_start_shutdown);
}
