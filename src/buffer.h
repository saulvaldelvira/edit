#ifndef BUFFER_H
#define BUFFER_H

#include "conf.h"
#include <stddef.h>
#include <time.h>
#include "history.h"
#include "vector.h"

struct buffer {
	int cx, cy;
	int rx;
	int row_offset, col_offset;
	int num_lines;
	wchar_t  *filename;
	vector_t *lines;
        history_t history;
	int dirty;
        struct buffer_conf conf;
        time_t last_auto_save;
};

extern struct buffers_data {
        struct buffer *curr;
        int amount;
        int curr_index;
} buffers;

void buffer_insert(void);
void buffer_clear(void);
int  buffer_drop(bool force);
void buffer_switch(int index);
int  buffer_current_index(void);
struct buffer* buffer_at(int index);

#define foreach_buffer(op) do { \
        int __index = buffers.curr_index; \
        for (int __i = 0; __i < buffers.amount; __i++) { buffer_switch(__i); op; } \
        buffer_switch(__index); } while (0)

#define current_buffer buffers.curr

#endif
