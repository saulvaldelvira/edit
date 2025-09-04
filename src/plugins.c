#include "plugins.h"
#include "console/io/output.h"
#include "definitions.h"
#include "prelude.h"
#include "log.h"
#include "init.h"
#include "vector.h"

#include <dlfcn.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>

static vector_t *plugins;

void init_plugins(void) {
        INIT_FUNC;

        plugins = vector_init(sizeof(void*), compare_pointer);
}

static int call_plugin(void *plugin, const char *func_name) {
        editor_set_status_message(L"Call");
        int (*func)() = (int (*)())dlsym(plugin, func_name);
        if (!func)
                die("No func");
        return func();
}

void reload_plugins(void) {
        for (size_t i = 0; i < vector_size(plugins); i++) {
                void **plugin = vector_at_ref(plugins, i);
                call_plugin(*plugin, "plugin_reload");
        }
}

int add_plugin(const char *path) {

        void *lib = dlopen(path, RTLD_LAZY);

        if (!lib) {
                editor_log(LOG_ERROR, "Failed to load \"%s\": %s", path, strerror(errno));
                return FAILURE;
        }

        vector_append(plugins, &lib);

        return call_plugin(lib, "plugin_init");
}
