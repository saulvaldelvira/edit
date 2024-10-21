#ifndef INIT_H
#define INIT_H

#define CLEANUP_FUNC do { \
        static bool _flag = false; \
        if (_flag) editor_log(LOG_WARN,"Cleanup function \"%s\" reached twice", __func__); \
        else editor_log(LOG_INFO, "SHUTDOWN: %s", __func__); \
        _flag = true; } while (0)

#include <log.h>

#define INIT_FUNC do { \
        editor_log(LOG_INFO,"INIT: %s", __func__); } while (0)

void init(void);

#endif
