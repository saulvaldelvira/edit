#ifndef INPUT_H
#define INPUT_H

#include "prelude.h"
#include "lib/GDS/src/LinkedList.h"

#define CTRL_KEY(k) ((k) & 0x1f)

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
	F1 = 0xFF1,
	F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12
};

int editor_read_key(void);
void editor_process_key_press(int c);
WString* editor_prompt(const wchar_t *prompt, const wchar_t *default_response, LinkedList *history);
bool editor_ask_confirmation(void);

#endif
