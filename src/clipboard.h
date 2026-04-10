#ifndef __CLIPBOARD_H__
#define __CLIPBOARD_H__

#include <wchar.h>

enum yank_kind {
        YANK_LINE,
        YANK_NORMAL,
};

void clipboard_push(const wchar_t *txt, size_t len);
void clipboard_clear();
void set_yank_kind(enum yank_kind);
enum yank_kind get_yank_kind();
const wchar_t* clipboard_get();

#endif
