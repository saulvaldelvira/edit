#ifndef PLATFORM_H
#define PLATFORM_H

void switch_ctrl_c(bool allow);
bool is_ctrl_c_pressed(void);

int get_window_size(int *rows, int *cols);

void enable_raw_mode(void);
void restore_termios(void);

char* get_default_eol(void);

#endif
