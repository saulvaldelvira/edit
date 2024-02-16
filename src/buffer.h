#ifndef BUFFER_H
#define BUFFER_H

#include "./lib/GDS/src/Vector.h"
#include <stddef.h>

struct buffer {
	int cx, cy;
	int rx;
	int row_offset, col_offset;
	int num_lines;
	wchar_t *filename;
	Vector *lines;
	int dirty;
	char *eol;
};

extern struct buffers_data {
        struct buffer *curr;
        int amount;
        int curr_index;
} buffers;

void buffer_init(void);
void buffer_insert(void);
void buffer_clear(void);
void buffer_drop(void);
void buffer_switch(int index);

#endif
