#include <prelude.h>
#include "conf.h"
#include "console/io.h"
#include "state.h"
#include "conf.h"
#include <init.h>

int main(int argc, char *argv[]){
        parse_args(argc, argv);
	init();
        parse_args_post_init();

        editor_refresh_screen(false);

	const int wait_timeout_ms = 30000;

	for (;;){
		int c = editor_read_key();
                received_key(c);
	       	editor_process_key_press(c);

		if (must_render_buffer()){
			editor_refresh_screen(false);
		}else if (must_render_stateline()){
			editor_refresh_screen(true);
		}

		if (c == NO_KEY){
                        editor_on_update();
			// Wait for 30 seconds, or until input is available
			if (wait_for_input(wait_timeout_ms))
				editor_refresh_screen(true);
		}
	}
}
