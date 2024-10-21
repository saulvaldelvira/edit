#ifndef __MACROS_H__
#define __MACROS_H__

#include "prelude.h"

void macro_start(wchar_t *name);

void macro_end(void);

void macro_play(wchar_t *name);

void macro_track_command(command_t cmd);

bool macro_is_recording(void);

#endif /* __MACROS_H__ */
