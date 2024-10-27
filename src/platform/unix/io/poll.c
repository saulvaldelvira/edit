#include "prelude.h"
#include "init.h"
#include "log.h"
#include <fcntl.h>
#include <prelude.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <sys/poll.h>
#include <unistd.h>
#include <wctype.h>
#include <signal.h>

static int pipefd[2];

static struct pollfd pollfds[2];
static const int poll_nfds = sizeof(pollfds) / sizeof(pollfds[0]);

static INLINE void wake_up_poll(void) {
        write(pipefd[1], " ", 1);
}

static void signal_handler(int sig){
	if (sig == SIGWINCH)
                wake_up_poll();
        signal(sig, signal_handler);
}

void init_poll(void) {
        INIT_FUNC;
        struct pollfd _initializer[2] = {
                {.fd = STDIN_FILENO, .events = POLLIN},
                {.fd = pipefd[0], .events = POLLIN},
        };
        memcpy(pollfds, _initializer, sizeof(_initializer));

        /* Prepare the pipe to "wake up" the editor on window resize.
	   The read end of the pipe must be non-blocking so the loop in main
	   that "discards" all written characters doesn't block the execution. */
	pipe(pipefd);
	signal(SIGWINCH, signal_handler);
	int flags = fcntl(pipefd[0], F_GETFL, 0);
	flags |= O_NONBLOCK;
	fcntl(pipefd[0], F_SETFL, flags);
}

bool wait_for_input(long ms) {
        poll(pollfds, poll_nfds, ms);
        if (pollfds[1].revents & POLLIN){
                char discard;
                while (read(pipefd[0], &discard, 1) == 1)
                        ;
                return true;
        }
        return false;
}

bool try_read_char(wint_t *dst) {
	static struct pollfd fds[1] = {
		{.fd = STDIN_FILENO, .events = POLLIN}
	};
	if (poll(fds, 1, 0) == 0)
		return false;

	*dst = getwchar();

        return true;
}
