#ifndef POLL_H
#define POLL_H

#include <stdbool.h>
#include <wchar.h>

bool wait_for_input(long ms);
bool try_read_char(wchar_t *dst);

#endif
