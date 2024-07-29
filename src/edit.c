#include "input.h"
#include "output.h"
#include "file.h"
#include "util.h"
#include "buffer.h"
#include "args.h"
#include "conf.h"
#include "cmd.h"
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <wchar.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
#include <poll.h>

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


static void cleanup(void){
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_term); // restore termios
	vector_free(conf.render);
	wprintf(L"\x1b[?1049l");
}

static int pipefd[2];

static void signal_handler(int sig){
	if (sig == SIGWINCH){
		write(pipefd[1], " ", 1);
		signal(SIGWINCH, signal_handler);
	}
}

static void init(void){
	if (get_window_size(&conf.screen_rows, &conf.screen_cols) == -1)
		die("get_window_size failed");
	conf.screen_rows -= BOTTOM_MENU_HEIGHT;
	conf.render = vector_init(sizeof(WString*), compare_equal);
	vector_set_destructor(conf.render, free_wstr);
	for (int i = 0; i < conf.screen_rows; i++){
		WString *wstr = wstr_empty();
		vector_append(conf.render, &wstr);
	}
        buffer_init();
	atexit(cleanup);
	setlocale(LC_CTYPE, "");
	enable_raw_mode();

	/* Prepare the pipe to "wake up" the editor on window resize.
	   The read end of the pipe must be non-blocking so the loop in main
	   that "discards" all written characters doesn't block the execution. */
	pipe(pipefd);
	signal(SIGWINCH, signal_handler);
	int flags = fcntl(pipefd[0], F_GETFL, 0);
	flags |= O_NONBLOCK;
	fcntl(pipefd[0], F_SETFL, flags);

	wprintf(L"\x1b[?1049h");
}

int main(int argc, char *argv[]){
	init();
        args_parse(argc,argv);

	buffer_switch(0);
        editor_refresh_screen(false);

        if (args.exec_cmd != NULL){
		WString *tmp = wstr_empty();
		wstr_concat_cstr(tmp, args.exec_cmd, -1);
		for (int i = 0; i < buffers.amount; i++){
			buffer_switch(i);
			editor_cmd(wstr_get_buffer(tmp));
			file_save(false, false);
		}
		wstr_free(tmp);
		exit(0);
	}

	struct pollfd pollfds[2] = {
		{.fd = STDIN_FILENO, .events = POLLIN},
		{.fd = pipefd[0], .events = POLLIN},
	};
	const int poll_nfds = sizeof(pollfds) / sizeof(pollfds[0]);
	const int poll_timeout_ms = 30000;

	int c = NO_KEY, last_c;
	long last_status_update = 0;

	for (;;){
		last_c = c;
		c = editor_read_key();
	       	editor_process_key_press(c);
		long curr = get_time_millis();

		if (c == NO_KEY && last_c != NO_KEY){
			editor_refresh_screen(false);
		}else if (curr - last_status_update > 500){
			editor_refresh_screen(true);
			last_status_update = curr;
		}

		if (c == NO_KEY){
			// Wait for 30 seconds, or until input is available
			poll(pollfds, poll_nfds, poll_timeout_ms);
			if (pollfds[1].revents & POLLIN){
				editor_refresh_screen(true);
				char discard;
				while (read(pipefd[0], &discard, 1) == 1);
			}
		}
	}
}
