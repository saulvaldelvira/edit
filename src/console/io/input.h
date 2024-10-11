#ifndef INPUT_H
#define INPUT_H

#include "prelude.h"
#include "poll.h"
#include "keys.h"
#include "linked_list.h"

void editor_process_key_press(int c);
const char* editor_prompt(const char *prompt, const char *default_response, linked_list_t *history);
bool editor_ask_confirmation(void);

#endif
