#include "buffer.h"
#include "buffer/highlight.h"
#include "buffer/line.h"
#include "console/cursor.h"
#include "init.h"
#include "lib/str/wstr.h"
#include "state.h"
#include "util.h"
#include <stdlib.h>
#include "render.h"

struct render render = {0};

static void __cleanup_render(void) {
        CLEANUP_FUNC;
        IGNORE_ON_FAST_CLEANUP (
                vector_free(render.buffer);
        )
}

void init_render(void) {
        atexit(__cleanup_render);

	render.buffer = vector_init(sizeof(wstring_t*), compare_equal);
	vector_set_destructor(render.buffer, free_wstr);
	for (int i = 0; i < state.screen_rows; i++){
		wstring_t *wstr = wstr_empty();
		vector_append(render.buffer, &wstr);
	}
}

selection_t render_get_selection(void) {
        if (!cursor_has_selection())
                return (selection_t) {0};
        selection_t sel = cursor_get_selection();
        wstring_t *line;
        vector_at(buffers.curr->lines, sel.end.y, &line);
        sel.end.x = line_cx_to_rx(line, sel.end.x);

        vector_at(buffers.curr->lines, sel.start.y, &line);
        sel.start.x = line_cx_to_rx(line, sel.start.x);
        return sel;
}

static void update_line(int cy){
	int i = cy - buffers.curr->row_offset;
	wstring_t *line;
	vector_at(render.buffer, i, &line);
	wstr_clear(line);
	wstring_t *row;
	vector_at(buffers.curr->lines, cy, &row);
	int rx = 0;

	for (size_t j = 0; j < wstr_length(row); j++){
		wchar_t c = wstr_get_at(row, j);
		if (c == L'\t'){
                        int tabwidth = get_character_width(L'\t', rx);
			for (int i = 0; i < tabwidth; i++)
				wstr_push_char(line, ' ');
		}else{
			wstr_push_char(line, c);
		}
		rx += get_character_width(c, rx);

	}
}

void editor_update_render(void){
	int i;
	for (i = 0; i < state.screen_rows; i++){
		int cy = buffers.curr->row_offset + i;
		if (cy >= buffers.curr->num_lines)
			break;
		update_line(cy);
	}
	for (; i < state.screen_rows; i++){
		wstring_t *line;
		vector_at(render.buffer, i, &line);
		wstr_clear(line);
	}
	if (buffers.curr->conf.syntax_highlighting)
		editor_highlight();
}
