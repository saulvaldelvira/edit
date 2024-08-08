#include <prelude.h>
#include <limits.h>
#include "buffer.h"
#include "util.h"
#include <prelude.h>
#include <conf.h>
#include <lib/json/src/json.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <file.h>
#include <time.h>
#include <sys/stat.h>
#include <cmd.h>
#include <log.h>

struct conf conf = {
        .quit_times = 3,
};

struct buffer_conf buffer_conf = {
        .tab_size = 8,
        .substitute_tab_with_space = false,
        .syntax_highlighting = false,
        .auto_save = true,
        .line_number = false,
        .eol = "\n",
};

static char* read_file(char *filename) {
        FILE *f = fopen(filename, "r");
        if (!f) return NULL;
        char *buf = NULL;
        size_t cap = 0, len = 0;

        int c;
        while ((c = fgetc(f)) != EOF) {
                if (len >= cap) {
                        cap *= 2;
                        if (cap == 0)
                                cap = 1024;
                        buf = realloc(buf, cap * sizeof(char));
                }
                buf[len++] = c;
        }
        buf[len] = '\0';
        fclose(f);
        return buf;
}

static json json_conf;
static bool must_free_conf = false;

static void __cleanup_conf(void) {
        if (must_free_conf)
                json_free(json_conf);
        must_free_conf = false;
}

static int parse_conf_file(char *filename) {
        static bool is_init = false;
        if (!is_init) {
                is_init = true;
                atexit(__cleanup_conf);
        }
        __cleanup_conf();

        char *text = read_file(filename);
        json_conf = json_deserialize(text);
        must_free_conf = true;
        free(text);

        if (json_conf.type != JSON_OBJECT) {
                if (json_conf.type == JSON_ERROR)
                        editor_log(LOG_ERROR, "Error parsing config file: %s\n", json_get_error_msg(json_conf.error_code));
                json_free(json_conf);
                must_free_conf = false;
                return -1;
        }

        #define VAL(field,_t,name) \
        if (strcmp(p.key, # field) == 0) {\
                if (p.val->type != _t) return -1; \
                CONF_STRUCT. field = p.val->name; \
        }

        #define VAL_BOOL(field) \
        if (strcmp(p.key, # field) == 0) {\
                if (p.val->type == JSON_TRUE) \
                        CONF_STRUCT. field = true; \
                else if (p.val->type == JSON_FALSE) \
                        CONF_STRUCT. field = false; \
                else return -1; \
        }

        for (size_t i = 0; i < json_conf.object.elems_len; i++) {
                struct pair p = json_conf.object.elems[i];
#define CONF_STRUCT buffer_conf
                VAL(tab_size,JSON_NUMBER,number);
                VAL_BOOL(substitute_tab_with_space);
                VAL_BOOL(syntax_highlighting);
                VAL_BOOL(auto_save);
                VAL_BOOL(line_number);
                VAL(eol,JSON_STRING,string);
#undef CONF_STRUCT
#define CONF_STRUCT conf
                VAL(quit_times,JSON_NUMBER,number);
#undef CONF_STRUCT
        }

        editor_log(LOG_INFO, "Loaded configuration from file %s\n", filename);
        return 1;
}

static char* default_conf_file(void) {
        static char conf_file[NAME_MAX];

        char *dir = getenv("XDG_CONFIG_HOME");
        if (!dir) {
                dir = getenv("HOME");
                if (!dir) return NULL;
                sprintf(conf_file, "%s/.config/edit/config.json", dir);
        } else {
                sprintf(conf_file, "%s/edit/config.json", dir);
        }

        return conf_file;
}

void conf_parse(int argc, char *argv[]) {
        struct args {
                char *exec_cmd;
        } args = {0};

	wchar_t filename[NAME_MAX];
	int i;

        char *conf_file = NULL;

	for (i = 1; i < argc && argv[i][0] == '-'; i++){
#define ARG(arg,next,if_found) \
                else if (strcmp(arg, argv[i]) == 0) {\
			if (next && i == argc - 1){\
				wprintf(L"\x1b[?1049l");\
				wprintf(L"Missing argument for \"%s\" command\n\r", arg);\
				exit(1);\
			}else{\
                                char *n = argv[++i];\
                                (void)n; \
                                if_found;\
			}\
                }
                if (false);
                ARG("--exec", true, {
				args.exec_cmd = n;
                                })
                ARG("--", false, { i++; break; })

                ARG("--conf-file", true, { conf_file = n;})
                ARG("--log-file", true, { set_log_file(n); })
                ARG("--log-level", true, {
                                int l = atoi(n);
                                set_log_level(l);
                        });
	}

        if (!conf_file)
                conf_file = default_conf_file();

        if (file_exists(conf_file))
                parse_conf_file(conf_file);
        else
                editor_log(LOG_WARN,"Conf file %s doesn't exist\n", conf_file);

	for (; i < argc; i++){
		buffer_insert();
		mbstowcs(filename, argv[i], NAME_MAX);
		filename[NAME_MAX-1] = '\0';
		if (file_open(filename) != 1)
			editor_end();
	}

        if (!buffers.curr)
                buffer_insert();

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
}
