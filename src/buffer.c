#include <prelude.h>
#include "buffer.h"
#include "conf.h"
#include "fs.h"
#include "init.h"
#include "lib/str/wstr.h"
#include "state.h"
#include "util.h"
#include <stdlib.h>
#include <stdio.h>
#include "console/io/output.h"

struct buffers_data buffers = {0};

static vector_t *buffers_vec;

static void free_buffer(void *e){
	struct buffer *buf = * (struct buffer**) e;
	free(buf->filename);
        vector_free(buf->lines);
        history_free(buf->history);
	free(buf);
}

static void __cleanup_buffer(void){
        CLEANUP_FUNC;
        IGNORE_ON_FAST_CLEANUP(
                vector_free(buffers_vec);
        );
}

void init_buffer(void){
        INIT_FUNC;
	buffers_vec = vector_init(sizeof(struct buffer*), compare_equal);
	vector_set_destructor(buffers_vec, free_buffer);
	buffers.curr_index = -1;
        atexit(__cleanup_buffer);
}

void buffer_insert(void){
	vector_t *lines = vector_init(sizeof(wstring_t*), compare_equal);
	vector_set_destructor(lines, free_wstr);

        buffers.curr = xmalloc(sizeof(struct buffer));

	*buffers.curr = (struct buffer) { .conf = buffer_conf };
	buffers.curr[0].lines = lines,
	buffers.curr[0].history = history_new();

	buffers.curr_index++;
	vector_insert_at(buffers_vec, buffers.curr_index, &buffers.curr);
	buffers.amount++;
}

void buffer_clear(void){
	vector_clear(buffers.curr->lines);
        history_clear(buffers.curr->history);
	buffers.curr->cx = 0;
	buffers.curr->cy = 0;
	buffers.curr->rx = 0;
	buffers.curr->col_offset = 0;
	buffers.curr->row_offset = 0;
	buffers.curr->num_lines = 0;
	buffers.curr->dirty = 0;
}

int buffer_drop(bool force){
        if (!force && buffers.curr->dirty) {
                editor_log(LOG_ERROR, "Can't close. Buffer has unsaved changes");
                editor_set_status_message(L"Can't close. Buffer has unsaved changes");
                return FAILURE;
        }
	char *tmp = get_tmp_filename();
	if (tmp)
		remove(tmp);
	free(tmp);
        vector_remove_at(buffers_vec, buffers.curr_index);
        if (buffers.curr_index == buffers.amount - 1)
                buffers.curr_index--;
        buffers.amount--;
        vector_at(buffers_vec, buffers.curr_index, &buffers.curr);
        if (buffers.amount == 0)
                editor_shutdown();
        return SUCCESS;
}

void buffer_switch(int index){
	if (index < 0 || index >= buffers.amount || index == buffers.curr_index)
		return;
	buffers.curr_index = index;
        vector_at(buffers_vec, buffers.curr_index, &buffers.curr);
}

INLINE
int buffer_current_index(void) {
        return buffers.curr_index;
}
