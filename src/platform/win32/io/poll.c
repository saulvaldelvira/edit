#include "prelude.h"
#include <winsock2.h>
#include "init.h"
#include "log.h"
#include <fcntl.h>
#include <prelude.h>
#include <winsock.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <wctype.h>

static int pipefd[2];

static struct pollfd pollfds[2];
static const int poll_nfds = sizeof(pollfds) / sizeof(pollfds[0]);

/* static INLINE void wake_up_poll(void) { */
/*         write(pipefd[1], " ", 1); */
/* } */

/* static void signal_handler(int sig){ */
/* 	if (sig == SIGWINCH) */
/*                 wake_up_poll(); */
/*         signal(sig, signal_handler); */
/* } */

#define STANDAR _fileno(stdin)

void init_poll(void) {
        INIT_FUNC;
        /* Prepare the pipe to "wake up" the editor on window resize.
	   The read end of the pipe must be non-blocking so the loop in main
	   that "discards" all written characters doesn't block the execution. */
	_pipe(pipefd, 256, O_BINARY);

        struct pollfd _initializer[2] = {
                {.fd = STANDAR, .events = POLLIN},
                {.fd = pipefd[0], .events = POLLIN},
        };
        memcpy(pollfds, _initializer, sizeof(_initializer));

	/* signal(SIGWINCH, signal_handler); */
        ioctlsocket(pipefd[0], FIONBIO, &(u_long){1});
}

bool wait_for_input(long ms) {
        WSAPoll(pollfds, poll_nfds, ms);
        if (pollfds[1].revents & POLLIN) {
                char discard;
                while (read(pipefd[0], &discard, 1) == 1)
                        ;
                return true;
        }
        return false;
}

bool try_read_char(wint_t *dst) {
	struct pollfd fds[1] = {
		{.fd = STANDAR, .events = POLLIN}
	};
	if (WSAPoll(fds, 1, 0) <= 0)
		return false;

	*dst = getwchar();

        return true;
}
