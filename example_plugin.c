/* Compile with: gcc example_plugin.c -ledit -fPIC -shared -o example_plugin.so -I src */
#include <assert.h>
#include <wchar.h>

#include "mapping.h"
#include "plugins.h"
#include "definitions.h"

extern void editor_set_status_message(const wchar_t *fmt, ...);

int map_reload(int nargs, command_arg_t *args) {
        (void)args;
        assert(nargs == 0);
        reload_plugins();
        return SUCCESS;
}

int plugin_init() {
        editor_set_status_message(L"Hi from plugin, I'll register a new mapping (CTRL + R)");

        register_mapping(
                        BUFFER_MODE_NORMAL,
                        (key_ty[]) {
                                KEY_CTRL('R')
                        },
                        1,
                        map_reload,
                        (command_arg_t[]){ (command_arg_t) {
                                .type = CMD_ARG_T_END,
                        }},
                        "Reload plugins"
        );

        return SUCCESS;
}

int plugin_reload() {
        editor_set_status_message(L"Reloading plugin");
        return SUCCESS;
}
