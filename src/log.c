#include <prelude.h>
#include "buffer.h"
#include "lib/GDS/src/Vector.h"
#include "lib/str/wstr.h"
#include "line.h"
#include "state.h"
#include "util.h"
#include <prelude.h>
#include <log.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <wchar.h>
#include <cursor.h>
#include <init.h>

static Vector *log_history;

static FILE *log_file;

static void __cleanup_log(void) {
        CLEANUP_FUNC;
        editor_log(LOG_INFO, "Goodbye :)");
        vector_free(log_history);
        if (log_file) {
                fclose(log_file);
                log_file = NULL;
        }
}

void init_log(void) {
        log_history = vector_init(sizeof(WString*), compare_equal);
        vector_set_destructor(log_history, free_wstr);
        atexit(__cleanup_log);
}


void set_log_file(char *fname) {
        if (log_file)
                fclose(log_file);
        log_file = fopen(fname, "w");
}

static enum log_level __log_level = LOG_INFO;

void set_log_level(enum log_level log_level) {
        __log_level = log_level;
}

void editor_log(enum log_level log_level, const char *fmt, ...) {
        if (log_level > __log_level)
                return;
        va_list ap;
        va_start(ap, fmt);
        size_t needed = vsnprintf(NULL, 0, fmt, ap) + 1;
        va_end(ap);

        static char timestamp[1024];
        double t = get_time_since_start_ms() / 1000.0;
        snprintf(timestamp, 1024, "[%.2f] ", t);

        char *buf = malloc(needed);

        va_start(ap, fmt);
        vsprintf(buf, fmt, ap);
        va_end(ap);

        WString *wstr = wstr_from_cstr(timestamp, 1024);
        wstr_concat_cstr(wstr, buf, needed);
        wstr_push_char(wstr, L'\n');

        wstr_replace(wstr, L"\n", L"");
        vector_append(log_history, &wstr);

        if (log_file) {
                fprintf(log_file, "%s", timestamp);
                fprintf(log_file, "%s\n", buf);
        }

        free(buf);
}

void view_log_buffer(void) {
        buffer_insert();
        size_t len = vector_size(log_history);
        for (size_t i = 0; i < len; i++) {
                WString *str;
                vector_at(log_history, i, &str);
                size_t str_len = wstr_length(str);
                const wchar_t *buf = wstr_get_buffer(str);
                line_append(buf, str_len);
        }
}
