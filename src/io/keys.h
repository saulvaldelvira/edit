#ifndef KEYS_H
#define KEYS_H

#include <wchar.h>

#define CTRL_KEY(k) ((k) & 0x1f)

extern wchar_t alt_key;

enum editor_key {
	NO_KEY = 9999,
	BACKSPACE = 127,
	ARROW_LEFT = 1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	CTRL_ARROW_LEFT,
	CTRL_ARROW_RIGHT,
	PAGE_UP,
	PAGE_DOWN,
	HOME_KEY,
	END_KEY,
	DEL_KEY,
	ALT_KEY,
        INSERT_KEY,
	F0 = 0xFF0, F1,
	F2, F3, F4, F5, F6, F7, F8,
        F9, F10, F11, F12, F13, F14,
        F15, F16,F17, F18, F19, F20
};

int editor_read_key(void);

#endif
