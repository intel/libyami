/*
 INTEL CONFIDENTIAL
 Copyright 2011 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
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
#endif /*  __VIDEO_ENCODER_LOG_H__ */
