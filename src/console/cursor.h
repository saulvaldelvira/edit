#ifndef CURSOR_H
#define CURSOR_H

#include <stdbool.h>

typedef enum cursor_direction {
        CURSOR_DIRECTION_UP = 1,
        CURSOR_DIRECTION_RIGHT,
        CURSOR_DIRECTION_DOWN,
        CURSOR_DIRECTION_LEFT,
        CURSOR_DIRECTION_START,
        CURSOR_DIRECTION_END,
        CURSOR_DIRECTION_PAGE_UP,
        CURSOR_DIRECTION_PAGE_DOWN,
} cursor_direction_t;

typedef struct coord {
        int x, y;
} coord_t;

typedef struct selection {
        coord_t start, end;
} selection_t;

void cursor_start_selection(void);
void cursor_stop_selection(void);
bool cursor_has_selection(void);
selection_t cursor_get_selection(void);
int cursor_move(cursor_direction_t key);
void cursor_goto(int x, int y);
void cursor_goto_start(void);
void cursor_goto_end(void);
void cursor_adjust(void);
int cursor_jump_word(cursor_direction_t dir);

#endif
