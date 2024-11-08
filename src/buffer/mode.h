#ifndef __BUFFER_MODE_H__
#define __BUFFER_MODE_H__

typedef enum buffer_mode {
        BUFFER_MODE_NORMAL = 0,
        BUFFER_MODE_INSERT,
        BUFFER_MODE_VISUAL,

        BUFFER_MODE_LEN
} buffer_mode_t;

buffer_mode_t buffer_mode_get_current(void);
void buffer_mode_set(buffer_mode_t mode);

#endif /* __BUFFER_MODE_H__ */
