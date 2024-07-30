#ifndef PRELUDE_H
#define PRELUDE_H

#define _POSIX_C_SOURCE 200112L
#define _XOPEN_SOURCE // wcwidth

#include <termios.h>
#include <wchar.h>
#include <limits.h>
#include "buffer.h"
#include <util.h>
#include "./lib/str/wstr.h"

#define SUCCESS 1
#define FAILURE 0

#define BOTTOM_MENU_HEIGHT 2
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#define TMP_EXT ".tmp"
#define MIN(a,b) ((a) < (b) ? (a) : (b))

#endif
