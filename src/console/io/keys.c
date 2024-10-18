#include <prelude.h>
#include "keys.h"
#include "log.h"
#include <wctype.h>
#include "poll.h"

wint_t seq[5];

int __alt_key(int k) {
       switch (k) {
       case 'A': // Alt + UP
                return ARROW_UP;
       case 'B': // Alt + DOWN
                return ARROW_DOWN;
       case 'C': // Alt + RIGHT
                return ARROW_RIGHT;
       case 'D': // Alt + LEFT
                return ARROW_LEFT;
       default:
                return k;
       }
}

key_ty alt_key(int k) {
        return (key_ty) {
                .k = __alt_key(k),
                .modif = KEY_MODIF_ALT,
        };
}

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

static key_ty keycode_seq(void) {
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
                return alt_key(keycode);
        }
        switch (keycode) {
        case 'C':
                if (ctrl) return KEY(CTRL_ARROW_RIGHT);
                else return KEY(ARROW_RIGHT);
        case 'D':
                if (ctrl) return KEY(CTRL_ARROW_LEFT);
                else return KEY(ARROW_LEFT);
        }
        return KEY('\x1b');
}

static key_ty control_seq_introducer(void) {
        if (seq[2] == L'~' || seq[3] == L'~')
                return KEY( vt_seq() );
        if (iswalpha(seq[2]) || seq[2] == ';')
                return keycode_seq();
        if (seq[1] >= 'A' && seq[1] <= 'H')
                return KEY( xterm_cursor_seq() );
        return KEY( '\x1b' );
}

static int single_shift_three(void) {
        switch (seq[1]){
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
        }
        return '\x1b';
}

static key_ty __editor_read_key(bool *is_seq){
        static wint_t c;
        if (!try_read_char(&c))
                return KEY(NO_KEY);
        if (c != L'\x1b') {
                editor_log(LOG_INPUT,"Received character: %d '%lc'", c, c);
                return KEY(c);
        }
        for (int i = 0; i < 5; i++) {
                if (!try_read_char(&seq[i]))
                        seq[i] = L' ';
        }
        *is_seq = true;
        if (seq[0] == L'[') {
                return control_seq_introducer();
        }else if (seq[0] == 'O'){
                return KEY( single_shift_three() );
        } else {
                return alt_key(seq[0]);
        }
        return KEY(c);
}

key_ty editor_read_key(void){
        bool is_seq = false;
        key_ty k = __editor_read_key(&is_seq);
        if (is_seq) {
                editor_log(LOG_INPUT,"Received key sequence: %c%c%c%c%c (%d, %d)",
                                     seq[0], seq[1], seq[2], seq[3], seq[4], k.k, k.modif);
        }
        return k;
}

const char* editor_get_key_repr(key_ty key) {
        switch (key.k) {
        case CTRL_KEY('q'):
                return "Ctrl + Q";
        case CTRL_KEY('o'):
                return "Ctrl + O";
        case F5:
                return "F5";
        }
        return "";
}
