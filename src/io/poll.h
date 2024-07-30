#ifndef POLL_H
#define POLL_H

bool wait_for_input(long ms);
bool try_read_char(wchar_t *dst);

#endif
