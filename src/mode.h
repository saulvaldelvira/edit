#ifndef MODE_H
#define MODE_H

#include <stddef.h>

extern wchar_t * mode_comments[];

enum {
	NO_MODE = 0, C_MODE, DEFAULT_MODE
};

int mode_get_current(void);

#endif // MODE_H
