/*
 *  VideoEncoderLog.h- basic va decoder for video
 *
 *  Copyright (C) 2013 Intel Corporation
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA
 */

#ifndef __VIDEO_ENCODER_LOG_H__
#define __VIDEO_ENCODER_LOG_H__

// Components
#include <utils/Log.h>


#ifdef ANDROID
#ifdef ANDROID_ALOG
#define mix_verboseLog(...) ALOGV(__VA_ARGS__)
#define mix_infoLog(...) ALOGI(__VA_ARGS__)
#define mix_warnLog(...) ALOGW(__VA_ARGS__)
#define mix_errorLog(...) ALOGE(__VA_ARGS__)
#elif ANDROID_LOG
#define mix_verboseLog(...) LOGV(__VA_ARGS__)
#define mix_infoLog(...) LOGI(__VA_ARGS__)
#define mix_warnLog(...) LOGW(__VA_ARGS__)
#define mix_errorLog(...) LOGE(__VA_ARGS__)
#endif
#else
#define mix_verboseLog(...) fprintf(stderr, __VA_ARGS__)
#define mix_infoLog(...) fprintf(stderr, __VA_ARGS__)
#define mix_warnLog(...) fprintf(stderr, __VA_ARGS__)
#define mix_errorLog(...) fprintf(stderr, __VA_ARGS__)
#endif

#define CHECK_VA_STATUS_RETURN(FUNC)\
    if (vaStatus != VA_STATUS_SUCCESS) {\
        mix_errorLog(FUNC" failed. vaStatus = %d\n", vaStatus);\
        return ENCODE_DRIVER_FAIL;\
    }

#define CHECK_VA_STATUS_GOTO_CLEANUP(FUNC)\
    if (vaStatus != VA_STATUS_SUCCESS) {\
        mix_errorLog(FUNC" failed. vaStatus = %d\n", vaStatus);\
        ret = ENCODE_DRIVER_FAIL; \
        goto CLEAN_UP;\
    }

#define CHECK_ENCODE_STATUS_RETURN(FUNC)\
    if (ret != ENCODE_SUCCESS) { \
        mix_errorLog(FUNC"Failed. ret = 0x%08x\n", ret); \
        return ret; \
    }

#define CHECK_ENCODE_STATUS_CLEANUP(FUNC)\
    if (ret != ENCODE_SUCCESS) { \
        mix_errorLog(FUNC"Failed, ret = 0x%08x\n", ret); \
        goto CLEAN_UP;\
    }

#define CHECK_NULL_RETURN_IFFAIL(POINTER)\
    if (POINTER == NULL) { \
        mix_errorLog("Invalid pointer\n"); \
        return ENCODE_NULL_PTR;\
    }
#endif                          /*  __VIDEO_ENCODER_LOG_H__ */
