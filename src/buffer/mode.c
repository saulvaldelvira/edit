#include "mode.h"

static buffer_mode_t __current_mode = BUFFER_MODE_INSERT;

buffer_mode_t buffer_mode_get_current(void) { return __current_mode; }

void buffer_mode_set(buffer_mode_t mode) {
        __current_mode = mode;
}
