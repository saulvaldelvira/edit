#ifndef POLL_H
#define POLL_H

#include <stdbool.h>
#include <wctype.h>

bool wait_for_input(long ms);
bool try_read_char(wint_t *dst);

#endif
