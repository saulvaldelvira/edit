#ifndef CURSOR_H
#define CURSOR_H

int cursor_move(int key);
void cursor_goto(int x, int y);
void cursor_adjust(void);
void cursor_jump_word(int key);

#endif
