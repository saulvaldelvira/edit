#include "clipboard.h"
#include "definitions.h"
#include "init.h"
#include "lib/str/wstr.h"
#include <stdlib.h>
#include <log.h>

static wstring_t *clipboard = NULL;
static enum yank_kind yk = YANK_NORMAL;

static void __clipboard_cleanup() {
        IGNORE_ON_FAST_CLEANUP(
                wstr_free(clipboard);
        )
}

void init_clipboard() {
        INIT_FUNC;
        clipboard = wstr_empty();
        atexit(__clipboard_cleanup);
}

void clipboard_clear() {
        wstr_clear(clipboard);
}

void set_yank_kind(enum yank_kind y) {
        yk = y;
}

enum yank_kind get_yank_kind() {
        return yk;
}

void clipboard_push(const wchar_t *txt, size_t len) {
        wstr_concat_cwstr(clipboard, txt, len);
}

const wchar_t* clipboard_get() {
        return wstr_get_buffer(clipboard);
}
