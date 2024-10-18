#ifndef INPUT_H
#define INPUT_H

#include "prelude.h"
#include "poll.h"
#include "keys.h"
#include "linked_list.h"

const wchar_t* editor_prompt(const wchar_t *prompt, const wchar_t *default_response, linked_list_t *history);
bool editor_ask_confirmation(void);

#endif
