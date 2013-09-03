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
   fprintf(stderr, "libvacodec error: "format"\n", ##__VA_ARGS__);\
}while (0)
#endif

#ifdef __ENABLE_DEBUG__
#ifndef INFO
#define INFO(format, ...)   do { \
   fprintf(stderr, "libvacodec info: "format"\n", ##__VA_ARGS__);\
}while (0)
#endif

#ifndef WARNING
#define WARNING(format, ...)   do { \
   fprintf(stderr, "libvacodec warning: "format"\n", ##__VA_ARGS__);\
}while (0)
#endif

#ifndef DEBUG
#define DEBUG(format, ...)   do { \
   fprintf(stderr, "libvacodec debug: "format"\n ", ##__VA_ARGS__);\
}while (0)
#endif

#else //__ENABLE_DEBUG__
#ifndef INFO
#define INFO(format, ...)
#endif

#ifndef WARNING
#define WARNING(format, ...)
#endif

#ifndef DEBUG
#define DEBUG(format, ...)
#endif
#endif //__ENABLE_DEBUG__

#endif //__ANDROID

#ifndef RETURN_IF_FAIL
#define RETURN_IF_FAIL(condition, val) \
do{ \
  if (!(condition)) \
     return (val);  \
}while(0)
#endif

static inline
bool vaapi_check_status(VAStatus status, const char *msg)
{
    if (status != VA_STATUS_SUCCESS) {
       ERROR("%s: %s", msg, vaErrorStr(status));
       return false;
    }
    return true;
}

#endif
