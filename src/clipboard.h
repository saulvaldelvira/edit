#ifndef __CLIPBOARD_H__
#define __CLIPBOARD_H__

#include <wchar.h>

void clipboard_push(const wchar_t *txt, size_t len);
void clipboard_clear();
const wchar_t* clipboard_get();

#endif
