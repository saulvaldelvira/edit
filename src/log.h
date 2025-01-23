#ifndef LOG_H
#define LOG_H

#include "console/io/output.h"
enum log_level {
        LOG_ERROR = 1,
        LOG_WARN = 2,
        LOG_INFO = 3,
        LOG_INPUT = 4,
};

void editor_log(enum log_level log_level, const char *fmt, ...);

void view_log_buffer(void);

void set_log_file(char *fname);
void set_log_level(enum log_level log_level);

char* last_logged_error(void);
char* last_logged_message(void);

#define log_and_display(level, fmt, ...) do {\
        editor_log(level, fmt __VA_OPT__(,) __VA_ARGS__); \
        editor_set_status_message(L"%s%s", \
                                  level == LOG_ERROR ? "ERROR: " : "", \
                                  last_logged_message()); \
} while (0)

#endif /* LOG_H */
