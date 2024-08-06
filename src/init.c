#include <prelude.h>
#include <init.h>
#include "conf.h"
#include "state.h"
#include "io/output.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <locale.h>
#include <stdlib.h>

static struct termios original_term;

static void enable_raw_mode(void){
	struct termios term;
	if (tcgetattr(STDIN_FILENO, &term) == -1)
		die("tcgetattr failed");
	original_term = term;
	term.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	term.c_oflag &= ~(OPOST);
	term.c_cflag |= (CS8);
	term.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	term.c_cc[VMIN] = 1;
	term.c_cc[VTIME] = 0;
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term) == -1)
		die("tcsetattr failed");
	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);
}


static void __cleanup(void){
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_term); // restore termios
	wprintf(L"\x1b[?1049l");
}

void init_state(void);
void init_buffer(void);
void init_cmd(void);
void init_file(void);
void init_io(void);

void init(void) {
        init_state();
        init_io();
        init_buffer();
        init_cmd();
        init_file();

	setlocale(LC_CTYPE, "");
	enable_raw_mode();

	wprintf(L"\x1b[?1049h");

        atexit(__cleanup);
}
