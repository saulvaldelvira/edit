#include <prelude.h>
#include "buffer.h"
#include "conf.h"
#include "file.h"
#include "lib/str/wstr.h"
#include "util.h"
#include <stdlib.h>
#include <stdio.h>

struct buffers_data buffers = {0};

static Vector *buffers_vec;

static void free_buffer(void *e){
	struct buffer *buf = * (struct buffer**) e;
	free(buf->filename);
        vector_free(buf->lines);
	free(buf);
}

static void cleanup_buffer(void){
        vector_free(buffers_vec);
}

void init_buffer(void){
        CLEANUP_GUARD;
	buffers_vec = vector_init(sizeof(struct buffer*), compare_equal);
	vector_set_destructor(buffers_vec, free_buffer);
	buffers.curr_index = -1;
        atexit(cleanup_buffer);
}

void buffer_insert(void){
	Vector *lines = vector_init(sizeof(WString*), compare_equal);
	vector_set_destructor(lines, free_wstr);

        buffers.curr = xmalloc(sizeof(struct buffer));

	*buffers.curr = (struct buffer) { .conf = buffer_conf };
	buffers.curr[0].lines = lines,

	buffers.curr_index++;
	vector_insert_at(buffers_vec, buffers.curr_index, &buffers.curr);
	buffers.amount++;
}

void buffer_clear(void){
	vector_clear(buffers.curr->lines);
	buffers.curr->cx = 0;
	buffers.curr->cy = 0;
	buffers.curr->rx = 0;
	buffers.curr->col_offset = 0;
	buffers.curr->row_offset = 0;
	buffers.curr->num_lines = 0;
	buffers.curr->dirty = 0;
}

void buffer_drop(void){
	char *tmp = get_tmp_filename();
	if (tmp)
		remove(tmp);
	free(tmp);
	if (buffers.amount == 1){
		wprintf(L"\x1b[2J\x1b[H");
		exit(0);
	}else{
		vector_remove_at(buffers_vec, buffers.curr_index);
		if (buffers.curr_index == buffers.amount - 1)
			buffers.curr_index--;
		buffers.amount--;
                vector_at(buffers_vec, buffers.curr_index, &buffers.curr);
	}
}

void buffer_switch(int index){
	if (index < 0 || index >= buffers.amount || index == buffers.curr_index)
		return;
	buffers.curr_index = index;
        vector_at(buffers_vec, buffers.curr_index, &buffers.curr);
}
