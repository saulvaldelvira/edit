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

	const int wait_timeout_ms = 30000;

        editor_force_render_screen();

	for (;;){
		key_ty c = editor_read_key();
                received_key(c);
	       	try_execute_action(c);

                editor_render_screen();

		if (c.k == NO_KEY){
                        editor_on_update();
			/* Wait for 30 seconds, or until input is available */
			wait_for_input(wait_timeout_ms);
		}
	}
}
