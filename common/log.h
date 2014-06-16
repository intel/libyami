#ifndef __COMMON_LOG_H__
#define __COMMON_LOG_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <va/va.h>

#ifdef ANDROID
#include <utils/Log.h>
#define ERROR(...)   ALOGE(__VA_ARGS__);
#define INFO(...)    ALOGI(__VA_ARGS__);
#define WARNING(...) ALOGV(__VA_ARGS__);
#define DEBUG(...)   ALOGV(__VA_ARGS__);
#else
#include <stdio.h>
#ifndef ERROR
#define ERROR(format, ...)   do { \
   fprintf(stderr, "libyami error(%s, %d): " format "\n", __func__, __LINE__, ##__VA_ARGS__);\
}while (0)
#endif

#ifdef __ENABLE_DEBUG__
#ifndef INFO
#define INFO(format, ...)   do { \
   fprintf(stderr, "libyami info(%s, %d): " format "\n",  __func__, __LINE__, ##__VA_ARGS__);\
}while (0)
#endif

#ifndef WARNING
#define WARNING(format, ...)   do { \
   fprintf(stderr, "libyami warning(%s, %d): " format "\n",  __func__, __LINE__, ##__VA_ARGS__);\
}while (0)
#endif

#ifndef DEBUG
#define DEBUG(format, ...)   do { \
   fprintf(stderr, "libyami debug(%s, %d): " format "\n",  __func__, __LINE__, ##__VA_ARGS__);\
}while (0)
#endif

#ifndef DEBUG_
#define DEBUG_(format, ...)   do { \
   fprintf(stderr, format, ##__VA_ARGS__);\
}while (0)
#endif

#else                           //__ENABLE_DEBUG__
#ifndef INFO
#define INFO(format, ...)
#endif

#ifndef WARNING
#define WARNING(format, ...)
#endif

#ifndef DEBUG
#define DEBUG(format, ...)
#endif
#ifndef DEBUG_
#define DEBUG_(format, ...)
#endif
#endif                          //__ENABLE_DEBUG__

#endif                          //__ANDROID

static inline bool checkVaapiStatus(VAStatus status, const char *msg)
{
    if (status != VA_STATUS_SUCCESS) {
        ERROR("%s: %s", msg, vaErrorStr(status));
        return false;
    }
    return true;
}

#endif
