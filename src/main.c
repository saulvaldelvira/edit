#include "io.h"
#include "conf.h"
#include "io/output.h"
#include "io/poll.h"
#include "log.h"
#include "util.h"
#include "conf.h"
#include <unistd.h>
#include <sys/time.h>
#include <init.h>

int main(int argc, char *argv[]){
	init();

        conf_parse(argc,argv);

        editor_refresh_screen(false);

	const int wait_timeout_ms = 30000;

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
			if (wait_for_input(wait_timeout_ms))
				editor_refresh_screen(true);
		}
	}
}
