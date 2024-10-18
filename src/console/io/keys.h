#ifndef KEYS_H
#define KEYS_H

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
        INSERT_KEY,
	F0 = 0xFF0, F1,
	F2, F3, F4, F5, F6, F7, F8,
        F9, F10, F11, F12, F13, F14,
        F15, F16,F17, F18, F19, F20,
};

enum key_modifier {
        KEY_MODIF_NONE = -1, /* TODO: Replace NO_KEY with this modifier */
        KEY_MODIF_NORMAL = 0,
        KEY_MODIF_ALT,
        KEY_MODIF_CTRL,
};

typedef struct key {
        int k;
        int modif;
} key_ty;

#define KEY(kv) (key_ty) { .k = kv }
#define KEY_ALT(kv) (key_ty) { .k = kv, .modif = KEY_MODIF_ALT }

key_ty editor_read_key(void);

const char* editor_get_key_repr(key_ty key);

#define is_key_printable(key) \
        ( ((key).modif == KEY_MODIF_NORMAL) && \
                (iswprint((key).k) || (key).k == '\t' || (key).k == '\r' || (key).k == '\n') )

#endif
