#include <prelude.h>
#include "keys.h"
#include "log.h"
#include <wctype.h>
#include "poll.h"

wint_t seq[5];
wchar_t alt_key;

static int xterm_cursor_seq(void) {
        switch (seq[1]) {
                case 'A': return ARROW_UP;
                case 'B': return ARROW_DOWN;
                case 'C': return ARROW_RIGHT;
                case 'D': return ARROW_LEFT;
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
                default: return '\x1b';
        }
}

static int vt_seq(void) {
        int code = seq[1] - L'0';
        if (iswdigit(seq[2]))
                code = code * 10 + ( seq[2] - L'0' );
        code--;
        static int arr[35] = {
                HOME_KEY, INSERT_KEY, DEL_KEY, END_KEY,
                PAGE_UP, PAGE_DOWN, HOME_KEY, END_KEY, NO_KEY,
                F0, F1, F2, F3, F4, F5, NO_KEY, F6, F7, F8, F9,
                F10, NO_KEY, F11, F12, F13, F14, NO_KEY, F15, F16,
                NO_KEY, F17, F18, F19, F20, NO_KEY
        };
        if (code >= 35) return '\x1b';
        return arr[code];
}

static int keycode_seq(void) {
        int modif = 1;
        int keycode = 1;
        if (seq[2] == ';') {
                /* [1;5D */
                keycode = seq[4];
                modif = seq[3] - L'0';
        } else {
                keycode = seq[2];
                modif = seq[1] - L'0';
        }
        bool ctrl = (modif - 1) == 4;
        if (modif == 3) {
                alt_key = keycode;
                return ALT_KEY;
        }
        switch (keycode) {
        case 'C':
                if (ctrl) return CTRL_ARROW_RIGHT;
                else return ARROW_RIGHT;
        case 'D':
                if (ctrl) return CTRL_ARROW_LEFT;
                else return ARROW_LEFT;
        }
        return '\x1b';
}

static int control_seq_introducer(void) {
        if (seq[2] == L'~' || seq[3] == L'~')
                return vt_seq();
        if (iswalpha(seq[2]) || seq[2] == ';')
                return keycode_seq();
        if (seq[1] >= 'A' && seq[1] <= 'H')
                return xterm_cursor_seq();
        return '\x1b';
}

static int single_shift_three(void) {
        switch (seq[1]){
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
        }
        return '\x1b';
}

int editor_read_key(void){
        static wint_t c;
        if (!try_read_char(&c))
                return NO_KEY;
        if (c != L'\x1b') {
                editor_log(LOG_INPUT,"Received character: %d '%lc'", c, c);
                return c;
        }
        for (int i = 0; i < 5; i++) {
                if (!try_read_char(&seq[i]))
                        seq[i] = L' ';
        }
        editor_log(LOG_INPUT,"Received key sequence: %c%c%c%c%c",
                             seq[0], seq[1], seq[2], seq[3], seq[4]);
        if (seq[0] == L'[') {
                return control_seq_introducer();
        }else if (seq[0] == 'O'){
                return single_shift_three();
        } else {
                /* ??? */
                alt_key = seq[0];
                return ALT_KEY;
        }
        return c;
}

