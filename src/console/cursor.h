#ifndef CURSOR_H
#define CURSOR_H

int cursor_move(int key);
void cursor_page_up(void);
void cursor_page_down(void);
void cursor_goto(int x, int y);
void cursor_adjust(void);
void cursor_jump_word(int key);

#endif
