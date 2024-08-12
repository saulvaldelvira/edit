#ifndef INPUT_H
#define INPUT_H

#include "prelude.h"
#include "poll.h"
#include "keys.h"
#include "lib/GDS/src/LinkedList.h"

void editor_process_key_press(int c);
const wchar_t* editor_prompt(const wchar_t *prompt, const wchar_t *default_response, LinkedList *history);
bool editor_ask_confirmation(void);

#endif
