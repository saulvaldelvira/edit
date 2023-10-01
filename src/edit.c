#include "edit.h"
#include "line.h"
#include "input.h"
#include "output.h"
#include "file.h"
#include "util.h"
#include "buffer.h"
#include "cmd.h"
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <wchar.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>

struct conf conf = {
	.tab_size = 8,
	.quit_times = 3,
	.status_msg[0] = '\0',
	.substitute_tab_with_space = false,
	.show_line_number = false
};

static void enable_raw_mode(void){
	struct termios term;
	if (tcgetattr(STDIN_FILENO, &term) == -1)
		die("tcgetattr, on enable_raw_mode");
	conf.original_term = term;
	term.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	term.c_oflag &= ~(OPOST);
	term.c_cflag |= (CS8);
	term.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	term.c_cc[VMIN] = 1;
	term.c_cc[VTIME] = 0;
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term) == -1)
		die("tcsetattr, on enable_raw_mode");
	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);
}

static void free_buffer(void *e){
	struct buffer *buf = (struct buffer*) e;
	free(buf->filename);
	if (buf->lines)
		vector_free(buf->lines);
}

static void cleanup(void){
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &conf.original_term); // restore termios
	vector_free(conf.buffers);
	free(conf.filename);
	vector_free(conf.lines_render);
	if (conf.lines)
		vector_free(conf.lines);
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
		die("get_window_size, at init_editor");
	conf.screen_rows -= 2;
	conf.lines_render = vector_init(sizeof(WString*), compare_equal);
	vector_set_destructor(conf.lines_render, free_wstr);
	for (int i = 0; i < conf.screen_rows; i++){
		WString *wstr = wstr_empty();
		vector_append(conf.lines_render, &wstr);
	}
	conf.buffers = vector_init(sizeof(struct buffer), compare_equal);
	vector_set_destructor(conf.buffers, free_buffer);
	atexit(cleanup);
	conf.buffer_index = -1; // Needed so "conf.buffer_index++" sets it to 0 on first buffer
	setlocale(LC_CTYPE, "");
	conf.last_auto_save = time(0);
	enable_raw_mode();

	pipe(pipefd);
	signal(SIGWINCH, signal_handler);
	int flags = fcntl(pipefd[1], F_GETFL, 0);
	flags |= O_NONBLOCK;
	fcntl(pipefd[1], F_SETFL, flags);

	flags = fcntl(pipefd[0], F_GETFL, 0);
	flags |= O_NONBLOCK;
	fcntl(pipefd[0], F_SETFL, flags);

	wprintf(L"\x1b[?1049h");
}

int main(int argc, char *argv[]){
	init();

	wchar_t filename[NAME_MAX];
	if (argc == 1){
		buffer_insert();
		editor_refresh_screen(false);
	}else{
		wprintf(L"\x1b[2J\x1b[H");
	}

	// Parse args (must be at the start)
	struct {
		char *exec_cmd;
	} args = {0};

	int i;
	for (i = 1; i < argc && argv[i][0] == '-'; i++){
		if (strcmp("--exec", argv[i]) == 0){
			if (i == argc - 1){
				wprintf(L"\x1b[?1049l");
				wprintf(L"Missing argument for \"--exec\" command\n\r");
				exit(1);
			}else{
				args.exec_cmd = argv[++i];
			}
		}else if (strcmp("--", argv[i]) == 0){
			i++;
			break;
		}
	}

	int first_buf = i-1;
	for (; i < argc; i++){
		editor_set_status_message(L"Loading buffer %d of %d [%s] ...", i - first_buf, argc - 1 - first_buf, argv[i]);
		buffer_insert();
		editor_refresh_screen(true);

		mbstowcs(filename, argv[i], NAME_MAX);
		filename[NAME_MAX-1] = '\0';
		if (file_open(filename) != 1)
			editor_end();
	}

	buffer_switch(0);

	if (args.exec_cmd != NULL){
		WString *tmp = wstr_empty();
		wstr_concat_cstr(tmp, args.exec_cmd, -1);
		for (int i = 0; i < conf.n_buffers; i++){
			buffer_switch(i);
			editor_cmd(wstr_get_buffer(tmp));
			file_save(false, false);
		}
		wstr_free(tmp);
		exit(0);
	}


	editor_set_status_message(L"");
	editor_refresh_screen(false);

	int c = NO_KEY, last_c;
	long last_status_update = 0;

	while (1){
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
			static struct timeval tv = {50,0};
			fd_set rfds;
			FD_ZERO(&rfds);
			FD_SET(STDIN_FILENO, &rfds);
			FD_SET(pipefd[0], &rfds);
			int max = (STDIN_FILENO > pipefd[0]) ? STDIN_FILENO : pipefd[0];
			max++;
			select(max, &rfds, NULL, NULL, &tv);
			char discard;
			while (read(pipefd[0], &discard, 1) == 1)
				editor_refresh_screen(true);
		}
	}
}

