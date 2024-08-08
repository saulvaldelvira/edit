#ifndef LOG_H
#define LOG_H

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

#endif /* LOG_H */
