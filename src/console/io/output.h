#ifndef OUTPUT_H
#define OUTPUT_H

#include "prelude.h"

void editor_refresh_screen(bool only_status_bar);
void editor_draw_rows(wstring_t *buf);
void editor_draw_status_bar(wstring_t *buf);
void editor_draw_message_bar(wstring_t *buf);
void editor_set_status_message(const wchar_t *fmt, ...);
void editor_help(void);

#endif

