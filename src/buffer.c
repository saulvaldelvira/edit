#include "buffer.h"
#include "line.h"
#include "file.h"
#include "input.h"
#include "util.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

static void save_buffer(void){
	struct buffer buffer;
	vector_get_at(conf.buffers, conf.buffer_index, &buffer);
	buffer.cx = conf.cx;
	buffer.cy = conf.cy;
	buffer.rx = conf.rx;
	buffer.row_offset = conf.row_offset;
	buffer.col_offset = conf.col_offset;
	buffer.num_lines = conf.num_lines;
	buffer.dirty = conf.dirty;
	free(buffer.filename);
	buffer.filename = conf.filename;
	buffer.lines = conf.lines;
	conf.lines = NULL;
	conf.filename = NULL;
	buffer.eol = conf.eol;
	vector_set_at(conf.buffers, conf.buffer_index, &buffer);
}

static void load_buffer(void){
	struct buffer buffer;
	vector_get_at(conf.buffers, conf.buffer_index, &buffer);
	if (conf.lines)
		vector_free(conf.lines);
	conf.lines = buffer.lines;
	buffer.lines = NULL;
	conf.cx = buffer.cx;
	conf.cy = buffer.cy;
	conf.rx = buffer.rx;
	conf.col_offset = buffer.col_offset;
	conf.row_offset = buffer.row_offset;
	free(conf.filename);
	conf.filename = buffer.filename;
	buffer.filename = NULL;
	conf.num_lines = buffer.num_lines;
	conf.dirty = buffer.dirty;
	conf.eol = buffer.eol;
	vector_set_at(conf.buffers, conf.buffer_index, &buffer);
}

void buffer_insert(void){
	Vector *lines = vector_init(sizeof(WString*), compare_equal);
	vector_set_destructor(lines, free_wstr);

	struct buffer buffer = {
		.lines = lines,
		.eol = DEFAULT_EOL
	};

	if (conf.n_buffers > 0)
		save_buffer();

	conf.buffer_index++;
	vector_insert_at(conf.buffers, conf.buffer_index, &buffer);

	load_buffer();
	conf.n_buffers++;
}

void buffer_clear(void){
	vector_clear(conf.lines);
	conf.cx = 0;
	conf.cy = 0;
	conf.rx = 0;
	conf.col_offset = 0;
	conf.row_offset = 0;
	conf.num_lines = 0;
	conf.dirty = 0;
}

void buffer_drop(void){
	char *tmp = get_tmp_filename();
	if (tmp)
		remove(tmp);
	free(tmp);
	if (conf.n_buffers == 1){
		wprintf(L"\x1b[2J\x1b[H");
		exit(0);
	}else{
		vector_remove_at(conf.buffers, conf.buffer_index);
		if (conf.buffer_index == conf.n_buffers - 1)
			conf.buffer_index--;
		conf.n_buffers--;
		load_buffer();
	}
}

void buffer_switch(int index){
	if (index < 0 || index >= conf.n_buffers)
		return;
	save_buffer();
	conf.buffer_index = index;
	load_buffer();
}

