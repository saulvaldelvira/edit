#include "mode.h"
#include "log.h"

static buffer_mode_t __current_mode = BUFFER_MODE_NORMAL;

buffer_mode_t buffer_mode_get_current(void) { return __current_mode; }

void buffer_mode_set(buffer_mode_t mode) {
        __current_mode = mode;
}

const char* buffer_mode_get_string(buffer_mode_t mode) {
        switch (mode) {
        case BUFFER_MODE_NORMAL:
                return "NORMAL";
        case BUFFER_MODE_INSERT:
                return "INSERT";
        case BUFFER_MODE_VISUAL:
                return "VISUAL";
        default:
                editor_log(LOG_ERROR, "%s: Invalid buffer_mode_t: %d", __func__, mode);
                return "";
        }
}
