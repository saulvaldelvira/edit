#ifndef __RENDER_H__
#define __RENDER_H__

#include "console/cursor.h"
#include "vector.h"

extern struct render {
	vector_t *buffer;
} render;

void editor_update_render(void);
selection_t render_get_selection(void);

#endif
