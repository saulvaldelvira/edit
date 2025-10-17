#include <prelude.h>
#include <platform.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <util.h>
#include <signal.h>
#include <sys/ioctl.h>

volatile int ctrl_c_pressed = 0;

INLINE bool is_ctrl_c_pressed(void) { return ctrl_c_pressed; }

static void ctrl_c_handler(int signal){
	if (signal == SIGINT)
		ctrl_c_pressed = 1;
}

void switch_ctrl_c(bool allow){
	static struct termios old, new;
	if (allow){
		tcgetattr(0, &old);
		new = old;
		new.c_lflag |= (ISIG);
		tcsetattr(0, TCSANOW, &new);
		signal(SIGINT, ctrl_c_handler);
		ctrl_c_pressed = 0;
	}else{
		tcsetattr(0, TCSANOW, &old);
		signal(SIGINT, SIG_DFL);
	}
}

int get_window_size(int *rows, int *cols){
	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
		wprintf(L"\x1b[999C\x1b[999B");
		return get_cursor_position(rows, cols);
	}else{
		*rows = ws.ws_row;
		*cols = ws.ws_col;
		return 0;
	}
}

static struct termios original_term;

void enable_raw_mode(void){
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

void restore_termios(void) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_term); // restore termios
}

INLINE char* get_default_eol(void) {
        return "\n";
}
