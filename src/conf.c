#include <prelude.h>
#include <limits.h>
#include "buffer.h"
#include "definitions.h"
#include "init.h"
#include "lib/str/wstr.h"
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
#include <wchar.h>
#include <fs.h>

struct conf conf = {
        .quit_times = 3,
};

struct buffer_conf buffer_conf = {
        .tab_size = 8,
        .substitute_tab_with_space = false,
        .syntax_highlighting = false,
        .auto_save_interval = 60,
        .line_number = false,
        .eol = PLATFORM_DEFAULT_EOL,
};

static json json_conf;
static bool must_free_json_conf = false;
static void __free_json(void) {
        if (must_free_json_conf) {
                json_free(json_conf);
                must_free_json_conf = false;
        }
}

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
        ONLY_ONCE( atexit(__free_json) );
        __free_json();
        char *text = read_file(filename);
        json_conf = json_deserialize(text);
        must_free_json_conf = true;
        free(text);

        if (json_conf.type != JSON_OBJECT) {
                if (json_conf.type == JSON_ERROR)
                        editor_log(LOG_ERROR, "Error parsing config file: %s", json_get_error_msg(json_conf.error_code));
                json_free(json_conf);
                return -1;
        }
        editor_log(LOG_INFO, "CONFIG: Parsing file %s", filename);

        #define VAL(field,_t,name,_fmt, ...) \
        if (strcmp(p.key, # field) == 0) {\
                if (p.val->type != _t) return -1; \
                CONF_STRUCT. field = p.val->name; \
                editor_log(LOG_INFO, "CONFIG: %s = " _fmt, #field, p.val->name); \
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
                editor_log(LOG_INFO, "CONFIG: %s = %s", #field, p.val->type ? "true" : "false"); \
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

        return SUCCESS;
}

static char* default_conf_file(void) {
        static char conf_file[NAME_MAX];
        snprintf(conf_file, NAME_MAX, "%s/config.json", get_config_directory());
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
static int argcount;
static char **argvec;

static struct args {
        char *exec_cmd;
} args = {0};

void parse_args(int argc, char *argv[]) {
        argcount = argc;
        argvec = argv;

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

/*
 * This is kind of an obsession of mine.
 * I dont like valgrind telling me I leak memory.
 * And if the cmd is "!quit", the program exits
 * without freeing the pointer allocated in the
 * function bellow.
 */
static wchar_t *cmd;
static void __free_args(void) {
        free(cmd);
}

void parse_args_post_init(void) {
        ONLY_ONCE( atexit(__free_args) );

        if (!conf_file)
                conf_file = default_conf_file();

        if (file_exists(conf_file)) {
                if (!parse_conf_file(conf_file))
                        exit(1);
        } else {
                editor_log(LOG_WARN,"Conf file %s doesn't exist", conf_file);
        }

        wchar_t filename[NAME_MAX];

	for (; i < argcount; i++){
		buffer_insert();
		mbstowcs(filename, argvec[i], NAME_MAX);
		filename[NAME_MAX-1] = '\0';
		if (file_open(filename) != 1)
			editor_end();
	}

        if (!buffers.curr)
                buffer_insert();

        if (args.exec_cmd != NULL){
                size_t len = strlen(args.exec_cmd) + 1;
                cmd = malloc(len * sizeof(wchar_t));
                mbstowcs(cmd, args.exec_cmd, len);

		foreach_buffer (
			editor_cmd(cmd);
                        if (buffers.curr->filename)
                                file_save(false, false);
                );
                free(cmd);
                 /* Make sure cmd is set to NULL, so it
                  * doesn't get freed twice */
                cmd = NULL;
		exit(0);
	}
}
