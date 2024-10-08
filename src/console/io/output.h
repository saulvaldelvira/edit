#ifndef OUTPUT_H
#define OUTPUT_H

#include "prelude.h"

void editor_refresh_screen(bool only_status_bar);
void editor_set_status_message(const wchar_t *fmt, ...);
void editor_help(void);

#endif

