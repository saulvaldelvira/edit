#include "prelude.h"

#include "input.h"
#include "init.h"
#include "lib/str/wstr.h"
#include "output.h"
#include "util.h"
#include "state.h"
#include <console.h>
#include <buffer/line.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <wchar.h>
#include <stdio.h>
#include <wctype.h>
#include "keys.h"
#include "poll.h"

#define RESPONSE_BUFS_SIZE 3

static wstring_t *responses[RESPONSE_BUFS_SIZE] = {0};
static uint8_t current_response = 0;

static void __cleanup_input(void) {
        CLEANUP_FUNC;
        for (size_t i = 0; i < RESPONSE_BUFS_SIZE; i++)
                wstr_free(responses[i]);
}

void init_input(void) {
        INIT_FUNC;
        for (size_t i = 0; i < RESPONSE_BUFS_SIZE; i++)
                responses[i] = wstr_empty();
        atexit(__cleanup_input);
}

static inline int __replace_with(wstring_t *wstr, wchar_t *new) {
        wstr_clear(wstr);
        if (new)
                wstr_concat_cwstr(wstr, new, -1);
        return wstr_length(wstr);
}

const wchar_t* editor_prompt(const wchar_t *prompt, const wchar_t *default_response, vector_t *history){
        size_t curr = current_response;
        current_response = (current_response + 1) % RESPONSE_BUFS_SIZE;
        return editor_prompt_ex(responses[curr], prompt, default_response, history);
}

const wchar_t* editor_prompt_ex(wstring_t *response, const wchar_t *prompt, const wchar_t *default_response, vector_t *history) {
        wstr_clear(response);

        vector_iterator_t it = {0};
        if (history)
                it = vector_iterator_from_back(history);

	if (default_response)
		wstr_concat_cwstr(response, default_response, FILENAME_MAX);

        if (!prompt)
                prompt = L"";

	size_t x = wstr_length(response);
	size_t base_x = wstrlen(prompt);
	int c = 'C';
        bool end = false;
        enum { UP, DOWN } direction = UP;
	while (!end){
		size_t offset = 0;
		if (base_x + x + 1 >= (size_t) state.screen_cols)
			offset = base_x + x + 1 - state.screen_cols;
		const wchar_t *buf = wstr_get_buffer(response);
		editor_set_status_message(L"%ls%ls", prompt, &buf[offset]);
		wprintf(L"\x1b[%d;%zuH", state.screen_rows + 2, base_x + x + 1);
		fflush(stdout);

                key_ty key = editor_read_key();
		c = key.k;
                if (key.modif & KEY_MODIF_CTRL)
                        c -= 64;

		switch (c)
		{
		case NO_KEY:
                        break;
		case DEL_KEY:
		case BACKSPACE:
			if (c == DEL_KEY){
				if (x < wstr_length(response))
					wstr_remove_at(response, x);
			}else if (x > 0){
				wstr_remove_at(response, x - 1);
				x--;
			}
			break;
		case CTRL_KEY(L'c'):
                case ESC:
			editor_set_status_message(L"");
			return NULL;
		case '\r':
                        end = true;
                        break;
		case CTRL_KEY(L'h'):
		case ARROW_LEFT:
			if (x > 0)
				x--;
			break;
		case CTRL_KEY(L'l'):
		case ARROW_RIGHT:
			if (x < wstr_length(response))
				x++;
			break;
                case ARROW_UP:
                        if (!history) break;
                        wchar_t *prev;
                        if (direction == DOWN) vector_it_prev(&it, &prev);
                        direction = UP;
                        if (vector_it_prev(&it, &prev) != NULL) {
                                x = __replace_with(response, prev);
                        }
                        break;
                case ARROW_DOWN:
                        if (!history) break;
                        wchar_t *next;
                        if (direction == UP) vector_it_next(&it, &next);
                        direction = DOWN;
                        if (vector_it_next(&it, &next) != NULL) {
                                x = __replace_with(response, next);
                        } else {
                                x = __replace_with(response, L"");
                        }
                        break;
		case HOME_KEY:
			x = 0;
			break;
		case END_KEY:
			x = wstr_length(response);
			break;
		default:
			if (iswprint(c)){
				wstr_insert(response, c, x);
				x++;
			}
			break;
		}
		if (c == NO_KEY){
                        wait_for_input(60000);
		}
	}
        editor_set_status_message(L"");
        if (history) {
                wchar_t *last;
                if (!vector_back(history, &last)
                        || wstr_cmp_cwstr(response, last) != 0)
                {
                        wchar_t *entry = wstr_cloned_cwstr(response);
                        vector_append(history, &entry);
                }
        }
        return wstr_get_buffer(response);
}

bool editor_ask_confirmation(const wchar_t *custom_prompt) {
        wchar_t msg[2000];
        swprintf(msg, 2000, L"%ls Y/n: ", custom_prompt ? custom_prompt : L"Are you sure?");
	const wchar_t*response = editor_prompt(msg, L"Y", NULL);
	bool result =
		response != NULL
		&& wstrlen(response) == 1
		&& (response[0] == L'Y'
		    || response[0] == L'y');
	return result;
}
