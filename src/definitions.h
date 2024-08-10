#ifndef __DEFINITIONS_H__
#define __DEFINITIONS_H__

#define SUCCESS 1
#define FAILURE 0

#define BOTTOM_MENU_HEIGHT 2
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#define TMP_EXT ".tmp"
#define MIN(a,b) ((a) < (b) ? (a) : (b))

#ifdef __GNUC__
#define INLINE inline __attribute__((always_inline))
#define NORETURN __attribute__((noreturn))
#else
#define INLINE inline
#define NORETURN
#endif

#endif
