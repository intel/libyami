#ifndef __LOG_H__
#define __LOG_H__

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
#define ERROR(...)   fprintf(stderr, __VA_ARGS__)
#define INFO(...)    fprintf(stderr, __VA_ARGS__)
#define WARNING(...) fprintf(stderr, __VA_ARGS__)
#define DEBUG(...)   fprintf(stderr, __VA_ARGS__)
#endif //ANDROID

#define RETURN_IF_FAIL(condition, val) \
do{ \
  if (!(condition)) \
     return (val);  \
}while(0)

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
