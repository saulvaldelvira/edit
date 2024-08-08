#ifndef CONF_H
#define CONF_H

#include <stdbool.h>
#include <time.h>

struct conf {
        int quit_times;
};

struct buffer_conf {
        char *eol;
        int tab_size;
        bool substitute_tab_with_space;
        time_t auto_save_interval;
        bool syntax_highlighting;
        bool line_number;
};

extern struct conf conf;
extern struct buffer_conf buffer_conf;

void conf_parse(int argc, char *argv[]);

#endif
