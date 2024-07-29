#ifndef ARGS_H
#define ARGS_H

extern struct args {
        char *exec_cmd;
} args;

void args_parse(int argc,char *argv[]);

#endif // ARGS_H
