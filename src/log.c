#include <prelude.h>
#include "buffer.h"
#include "vector.h"
#include "lib/str/wstr.h"
#include <buffer/line.h>
#include "state.h"
#include "util.h"
#include <prelude.h>
#include <log.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <wchar.h>
#include <console.h>
#include <init.h>

static vector_t *log_history;

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
        log_history = vector_init(sizeof(wstring_t*), compare_equal);
        vector_set_destructor(log_history, free_wstr);
        atexit(__cleanup_log);
}


bool set_log_file(char *fname) {
        if (log_file)
                fclose(log_file);
        log_file = fopen(fname, "w");
        return log_file != NULL;
}

static enum log_level __log_level = LOG_INFO;

void set_log_level(enum log_level log_level) {
        __log_level = log_level;
}

#define LAST_LOGGED_ERROR_LEN 1024
static char LAST_LOGGED_ERROR[LAST_LOGGED_ERROR_LEN];

#define LAST_LOGGED_MSG_LEN 1024
static char LAST_LOGGED_MSG[LAST_LOGGED_MSG_LEN];

void editor_log(enum log_level log_level, const char *fmt, ...) {
        if (log_level > __log_level)
                return;
        va_list ap;
        va_start(ap, fmt);
        size_t needed = vsnprintf(NULL, 0, fmt, ap) + 1;
        va_end(ap);

        char *title = "";
        if (log_level == LOG_ERROR)
                title = "ERROR: ";

        static char timestamp[1024];
        double t = get_time_since_start_ms() / 1000.0;
        snprintf(timestamp, 1024, "[%.2f] %s", t, title);

        char *buf = malloc(needed);

        va_start(ap, fmt);
        vsprintf(buf, fmt, ap);
        va_end(ap);

        if (log_level == LOG_ERROR) {
                va_start(ap, fmt);
                vsnprintf(LAST_LOGGED_ERROR, LAST_LOGGED_ERROR_LEN, fmt, ap);
                va_end(ap);
        }

        va_start(ap, fmt);
        vsnprintf(LAST_LOGGED_MSG, LAST_LOGGED_MSG_LEN, fmt, ap);
        va_end(ap);

        wstring_t *wstr = wstr_from_cstr(timestamp, 1024);
        wstr_concat_cstr(wstr, buf, needed);

        /* TODO: Replace more escape characters  */
        wstr_replace(wstr, L"\n", L"\\n");
        wstr_replace(wstr, L"\r", L"\\r");
        vector_append(log_history, &wstr);

        if (log_file) {
                fprintf(log_file, "%s", timestamp);
                fprintf(log_file, "%s\n", buf);
        }

        free(buf);
}

char* last_logged_error(void) {
        return LAST_LOGGED_ERROR;
}

char* last_logged_message(void) {
        return LAST_LOGGED_MSG;
}

void view_log_buffer(void) {
        buffer_insert();
        size_t len = vector_size(log_history);
        for (size_t i = 0; i < len; i++) {
                wstring_t *str;
                vector_at(log_history, i, &str);
                size_t str_len = wstr_length(str);
                const wchar_t *buf = wstr_get_buffer(str);
                line_append(buf, str_len);
        }
}
