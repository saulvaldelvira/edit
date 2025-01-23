#include <prelude.h>
#include "state.h"
#include <stdlib.h>
#include <init.h>

void init_state(void);
void init_buffer(void);
void init_cmd(void);
void init_fs(void);
void init_io(void);
void init_log(void);
void init_mapping(void);

void init(void) {
        init_log();
        init_state();

        init_io();
        init_buffer();
        init_cmd();
        init_fs();
        init_mapping();
}
