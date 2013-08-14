#ifndef __LOG_H__
#define __LOG_H__

#ifdef ANDROID
#include <utils/Log.h>
#define LOG_ERROR(...)   ALOGE(__VA_ARGS__);
#define LOG_INFO(...)    ALOGI(__VA_ARGS__);
#define LOG_WARNING(...) ALOGV(__VA_ARGS__);
#define LOG_DEBUG(...)   ALOGV(__VA_ARGS__);
#else
#include <stdio.h>
#define LOG_ERROR(...)   fprintf(stderr, __VA_ARGS__)
#define LOG_INFO(...)    fprintf(stderr, __VA_ARGS__)
#define LOG_WARNING(...) fprintf(stderr, __VA_ARGS__)
#define LOG_DEBUG(...)   fprintf(stderr, __VA_ARGS__)
#endif

#define RETURN_IF_FAIL(condition) \
    if (!(condition)) \
      return;

#define RETURN_VAL_IF_FAIL(condition, value) \
    if (!(condition)) \
      return (value);

#endif
