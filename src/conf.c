#include <prelude.h>
#include <limits.h>
#include "buffer.h"
#include "state.h"
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
        .auto_save_interval = 60,
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

static int parse_conf_file(char *filename) {
        static json json_conf;
        char *text = read_file(filename);
        json_conf = json_deserialize(text);
        free(text);

        if (json_conf.type != JSON_OBJECT) {
                if (json_conf.type == JSON_ERROR)
                        editor_log(LOG_ERROR, "Error parsing config file: %s", json_get_error_msg(json_conf.error_code));
                json_free(json_conf);
                return -1;
        }
        editor_log(LOG_INFO, "Parsing configuration from file %s", filename);

        #define VAL(field,_t,name,_fmt, ...) \
        if (strcmp(p.key, # field) == 0) {\
                if (p.val->type != _t) return -1; \
                CONF_STRUCT. field = p.val->name; \
                editor_log(LOG_INFO, "CONFIG: Override %s = " _fmt  "\n", #field, p.val->name); \
                found = true; \
                __VA_ARGS__ \
        }

        #define VAL_STR(field, ...) VAL(field,JSON_STRING,string,"%s", __VA_ARGS__)
        #define VAL_NUM(field, ...) VAL(field,JSON_NUMBER,number,"%f", __VA_ARGS__)

        #define VAL_BOOL(field) \
        if (strcmp(p.key, # field) == 0) {\
                if (p.val->type == JSON_TRUE) \
                        CONF_STRUCT. field = true; \
                else if (p.val->type == JSON_FALSE) \
                        CONF_STRUCT. field = false; \
                else return -1; \
                editor_log(LOG_INFO, "Override %s = %s\n", #field, p.val->type ? "true" : "false"); \
                found = true; \
        }

        for (size_t i = 0; i < json_conf.object.elems_len; i++) {
                struct pair p = json_conf.object.elems[i];
                bool found = false;
#define CONF_STRUCT buffer_conf
                VAL_NUM(tab_size);
                VAL_BOOL(substitute_tab_with_space);
                VAL_BOOL(syntax_highlighting);
                VAL_NUM(auto_save_interval);
                VAL_BOOL(line_number);
                VAL_STR(eol);
#undef CONF_STRUCT
#define CONF_STRUCT conf
                VAL_NUM(quit_times);
#undef CONF_STRUCT

                if (!found) {
                        editor_log(LOG_WARN, "Unknown config key: %s", p.key);
                }
        }

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

static NORETURN void help(void) {
        fprintf(stderr,
"edit - Terminal-based text editor\n"
"USAGE: edit [--exec <command>] [--conf-file <file>] [--log-file <file>]\n"
"            [--log-level <number>] [--help] [filename1..filenameN]\n"
"\n"
"OPTIONS\n"
"--exec <command>, Executes <command> for every file given as a parameter.\n"
"\n"
"--conf-file <file>, Specify a config file\n"
"\n"
"--log-file <file>, Specify a log file\n"
"\n"
"--log-level <number>, Set the log level. ERROR (1), WARN (2), INFO (3), INPUT (4)\n"
"\n"
"--, Breaks the argument parsing. Treats the rest of the arguments as filenames.\n"
"\n"
"EXAMPLES\n"
"Open files for editing: edit my-file.txt another-file.c\n"
"\n"
"Strip trailing spaces from files: edit --exec \"strip buffer\" *.txt\n");
        exit(1);
}


static char *conf_file = NULL;
static int i;
static int __argc;
static char **__argv;

static struct args {
        char *exec_cmd;
} args = {0};

void parse_args(int argc, char *argv[]) {
        __argc = argc;
        __argv = argv;


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
                        })
                ARG("--help", false, { help(); });
	}

}

void parse_args_post_init(void) {
        if (!conf_file)
                conf_file = default_conf_file();

        if (file_exists(conf_file)) {
                if (!parse_conf_file(conf_file))
                        exit(1);
        } else {
                editor_log(LOG_WARN,"Conf file %s doesn't exist", conf_file);
        }

        wchar_t filename[NAME_MAX];

	for (; i < __argc; i++){
		buffer_insert();
		mbstowcs(filename, __argv[i], NAME_MAX);
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
