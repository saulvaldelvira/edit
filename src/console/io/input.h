#ifndef INPUT_H
#define INPUT_H

#include "prelude.h"
#include "poll.h"

const wchar_t* editor_prompt(const wchar_t *prompt, const wchar_t *default_response, vector_t *history);
const wchar_t* editor_prompt_ex(wstring_t *buf, const wchar_t *prompt, const wchar_t *default_response, vector_t *history);
bool editor_ask_confirmation(const wchar_t *custom_prompt);

#endif
