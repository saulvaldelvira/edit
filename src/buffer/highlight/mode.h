#ifndef MODE_H
#define MODE_H

#include <stddef.h>

extern wchar_t * mode_comments[][2];

enum {
	NO_MODE = 0, C_MODE, DEFAULT_MODE, HTML_MODE
};

int highligth_mode_current(void);

#endif // MODE_H
