/*
 *  VideoEncoderDefs.h- basic va decoder for video
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

#ifndef __VIDEO_ENCODER_DEF_H__
#define __VIDEO_ENCODER_DEF_H__

// config.h should NOT be included in header file, especially for the header file used by external
// XXX __ENABLE_CAPI__ still seems ok in this file

#include <va/va.h>
#include <stdint.h>
#include "VideoCommonDefs.h"

#define STRING_TO_FOURCC(format) ((uint32_t)(((format)[0])|((format)[1]<<8)|((format)[2]<<16)|((format)[3]<<24)))

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t Encode_Status;
typedef void * Yami_PTR;
// Video encode error code
enum {
    ENCODE_NO_REQUEST_DATA = -10,
    ENCODE_WRONG_STATE = -9,
    ENCODE_NOTIMPL = -8,
    ENCODE_NO_MEMORY = -7,
    ENCODE_NOT_INIT = -6,
    ENCODE_DRIVER_FAIL = -5,
    ENCODE_INVALID_PARAMS = -4,
    ENCODE_NOT_SUPPORTED = -3,
    ENCODE_NULL_PTR = -2,
    ENCODE_FAIL = -1,
    ENCODE_SUCCESS = 0,
    ENCODE_ALREADY_INIT = 1,
    ENCODE_SLICESIZE_OVERFLOW = 2,
    ENCODE_BUFFER_TOO_SMALL = 3, // The buffer passed to encode is too small to contain encoded data
    ENCODE_BUFFER_NO_MORE = 4,   //No more output buffers.
    ENCODE_IS_BUSY = 5, // driver is busy, there are too many buffers under encoding in parallel.
};

typedef enum {
    OUTPUT_EVERYTHING = 0,      //Output whatever driver generates
    OUTPUT_CODEC_DATA = 1,      // codec data for mp4, similar to avcc format
    OUTPUT_FRAME_DATA = 2,      //Equal to OUTPUT_EVERYTHING when no header along with the frame data
    OUTPUT_ONE_NAL = 4,
    OUTPUT_ONE_NAL_WITHOUT_STARTCODE = 8,
    OUTPUT_LENGTH_PREFIXED = 16,
    OUTPUT_BUFFER_LAST
}VideoOutputFormat;

typedef enum {
    // TODO, USE VA Fourcc here
    RAW_FORMAT_NONE = 0,
    RAW_FORMAT_YUV420 = 1,
    RAW_FORMAT_YUV422 = 2,
    RAW_FORMAT_YUV444 = 4,
    RAW_FORMAT_NV12 = 8,
    RAW_FORMAT_PROTECTED = 0x80000000,
    RAW_FORMAT_LAST
}VideoRawFormat;

typedef enum {
    RATE_CONTROL_NONE = VA_RC_NONE,
    RATE_CONTROL_CBR = VA_RC_CBR,
    RATE_CONTROL_VBR = VA_RC_VBR,
    RATE_CONTROL_VCM = VA_RC_VCM,
    RATE_CONTROL_CQP = VA_RC_CQP,
    RATE_CONTROL_LAST
}VideoRateControl;

typedef enum {
    PROFILE_MPEG2SIMPLE = 0,
    PROFILE_MPEG2MAIN,
    PROFILE_MPEG4SIMPLE,
    PROFILE_MPEG4ADVANCEDSIMPLE,
    PROFILE_MPEG4MAIN,
    PROFILE_H264BASELINE,
    PROFILE_H264MAIN,
    PROFILE_H264HIGH,
    PROFILE_VC1SIMPLE,
    PROFILE_VC1MAIN,
    PROFILE_VC1ADVANCED,
    PROFILE_H263BASELINE
}VideoProfile;

typedef enum {
    AVC_DELIMITER_LENGTHPREFIX = 0,
    AVC_DELIMITER_ANNEXB
}AVCDelimiterType;

typedef enum {
    VIDEO_ENC_NONIR,            // Non intra refresh
    VIDEO_ENC_CIR,              // Cyclic intra refresh
    VIDEO_ENC_AIR,              // Adaptive intra refresh
    VIDEO_ENC_BOTH,
    VIDEO_ENC_LAST
}VideoIntraRefreshType;

typedef enum {
    BUFFER_SHARING_NONE = 1,    //Means non shared buffer mode
    BUFFER_SHARING_CI = 2,
    BUFFER_SHARING_V4L2 = 4,
    BUFFER_SHARING_SURFACE = 8,
    BUFFER_SHARING_USRPTR = 16,
    BUFFER_SHARING_GFXHANDLE = 32,
    BUFFER_SHARING_KBUFHANDLE = 64,
    BUFFER_LAST
}VideoBufferSharingMode;

typedef enum {
    AVC_STREAM_FORMAT_AVCC,
    AVC_STREAM_FORMAT_ANNEXB
}AVCStreamFormat;

// Output buffer flag
#define ENCODE_BUFFERFLAG_ENDOFFRAME       0x00000001
#define ENCODE_BUFFERFLAG_PARTIALFRAME     0x00000002
#define ENCODE_BUFFERFLAG_SYNCFRAME        0x00000004
#define ENCODE_BUFFERFLAG_CODECCONFIG      0x00000008
#define ENCODE_BUFFERFLAG_DATACORRUPT      0x00000010
#define ENCODE_BUFFERFLAG_DATAINVALID      0x00000020
#define ENCODE_BUFFERFLAG_SLICEOVERFOLOW   0x00000040

typedef struct VideoEncOutputBuffer {
    uint8_t *data;
    uint32_t bufferSize;        //buffer size
    uint32_t dataSize;          //actuall size
    uint32_t remainingSize;
    uint32_t flag;                   //Key frame, Codec Data etc
    VideoOutputFormat format;   //output format
    uint64_t timeStamp;         //reserved
#ifndef __ENABLE_CAPI__
     VideoEncOutputBuffer():data(0), bufferSize(0), dataSize(0)
    , remainingSize(0), flag(0), format(OUTPUT_BUFFER_LAST), timeStamp(0) {
    };
#endif
}VideoEncOutputBuffer;

#ifdef __BUILD_GET_MV__
    /*
    * VideoEncMVBuffer is defined to store Motion vector.
    * Memory size allocated to data pointer can be got by getMVBufferSize API.
    * Picture is partitioned to 16*16 block, within the 16x16 block, each 4x4 subblock has the same MV.
    * Access the first 4*4 subblock to get the MV of each 16*16 block, and skip the 16*16 block to
    * get MV of next 16x16 Block.
    * Refer to structure VAMotionVectorIntel about detailed layout for MV of 4*4 subblock.
    * 16x16 block is in raster scan order, within the 16x16 block, each 4x4 block MV is ordered as below in memory.
    *                         16x16 Block
    *        -----------------------------------------
    *        |    1    |    2    |    5     |    6    |
    *        -----------------------------------------
    *        |    3    |    4    |    7     |    8    |
    *        -----------------------------------------
    *        |    9    |    10   |    13   |    14   |
    *        -----------------------------------------
    *        |    11   |    12   |    15   |    16   |
    *        -----------------------------------------
    */
typedef struct VideoEncMVBuffer {
    uint8_t *data;                //memory to store the MV
    uint32_t bufferSize;        //buffer size
    uint32_t dataSize;          //actuall size
#ifndef __ENABLE_CAPI__
     VideoEncMVBuffer():data(0), bufferSize(0), dataSize(0) {
    };
#endif
}VideoEncMVBuffer;
#endif

typedef struct VideoEncRawBuffer {
    uint8_t *data;
    uint32_t fourcc;
    uint32_t size;
    bool bufAvailable;          //To indicate whether this buffer can be reused
    uint64_t timeStamp;         //reserved
    bool forceKeyFrame;
    intptr_t id;
#ifndef __ENABLE_CAPI__
     VideoEncRawBuffer():data(0), fourcc(0), size(0), bufAvailable(false),
        timeStamp(0), forceKeyFrame(false), id(-1) {
    };
#endif
}VideoEncRawBuffer;

typedef struct VideoEncSurfaceBuffer {
    VASurfaceID surface;
    uint8_t *usrptr;
    uint32_t index;
    bool bufAvailable;
    struct EncSurfaceBuffer *next;
#ifndef __ENABLE_CAPI__
    VideoEncSurfaceBuffer():surface(VA_INVALID_ID), usrptr(0)
    , index(0), bufAvailable(false) {
    };
#endif
}VideoEncSurfaceBuffer;

typedef struct AirParams {
    uint32_t airMBs;
    uint32_t airThreshold;
    uint32_t airAuto;
#ifndef __ENABLE_CAPI__
     AirParams():airMBs(0), airThreshold(0), airAuto(0) {
    };
#endif
}AirParams;

typedef struct VideoFrameRate {
    uint32_t frameRateNum;
    uint32_t frameRateDenom;
#ifndef __ENABLE_CAPI__
     VideoFrameRate():frameRateNum(0), frameRateDenom(1) {
    };
#endif
}VideoFrameRate;

typedef struct VideoResolution {
    uint32_t width;
    uint32_t height;
#ifndef __ENABLE_CAPI__
     VideoResolution():width(0), height(0) {
    };
#endif
}VideoResolution;

typedef struct VideoRateControlParams {
    uint32_t bitRate;
    uint32_t initQP;
    uint32_t minQP;
    uint32_t maxQP;
    uint32_t windowSize; // use for HRD CPB length in ms
    uint32_t targetPercentage;
    uint32_t disableFrameSkip;
    uint32_t disableBitsStuffing;
#ifndef __ENABLE_CAPI__
     VideoRateControlParams():bitRate(0), initQP(0), minQP(0)
    , windowSize(0), targetPercentage(0)
    , disableFrameSkip(0), disableBitsStuffing(0) {
    };
#endif
}VideoRateControlParams;

typedef struct SliceNum {
    uint32_t iSliceNum;
    uint32_t pSliceNum;
#ifndef __ENABLE_CAPI__
     SliceNum():iSliceNum(1), pSliceNum(1) {
    };
#endif
}SliceNum;

typedef struct ExternalBufferAttrib {
    uint32_t realWidth;
    uint32_t realHeight;
    uint32_t lumaStride;
    uint32_t chromStride;
    uint32_t format;
} ExternalBufferAttrib;

typedef struct Cropping {
    uint32_t LeftOffset;
    uint32_t RightOffset;
    uint32_t TopOffset;
    uint32_t BottomOffset;
#ifndef __ENABLE_CAPI__
     Cropping():LeftOffset(0), RightOffset(0),
        TopOffset(0), BottomOffset(0) {
    };
#endif
}Cropping;

typedef struct SamplingAspectRatio {
    uint16_t SarWidth;
    uint16_t SarHeight;
#ifndef __ENABLE_CAPI__
     SamplingAspectRatio():SarWidth(0), SarHeight(0) {
    };
#endif
}SamplingAspectRatio;

typedef enum {
    VideoParamsTypeStartUnused = 0x01000000,
    VideoParamsTypeCommon,
    VideoParamsTypeAVC,
    VideoParamsTypeH263,
    VideoParamsTypeMP4,
    VideoParamsTypeVC1,
    VideoParamsTypeUpSteamBuffer,
    VideoParamsTypeUsrptrBuffer,
    VideoParamsTypeHRD,
    VideoParamsTypeStoreMetaDataInBuffers,

    VideoConfigTypeFrameRate,
    VideoConfigTypeBitRate,
    VideoConfigTypeResolution,
    VideoConfigTypeIntraRefreshType,
    VideoConfigTypeAIR,
    VideoConfigTypeCyclicFrameInterval,
    VideoConfigTypeAVCIntraPeriod,
    VideoConfigTypeNALSize,
    VideoConfigTypeIDRRequest,
    VideoConfigTypeSliceNum,

    //format related
    VideoConfigTypeAVCStreamFormat,

    VideoParamsConfigExtension
}VideoParamConfigType;

typedef struct VideoParamConfigSet {
    uint32_t size;
}VideoParamConfigSet;

typedef struct VideoParamsCommon {
    uint32_t size;
    VAProfile profile;
    uint8_t level;
    VideoRawFormat rawFormat;
    VideoResolution resolution;
    VideoFrameRate frameRate;
    uint32_t intraPeriod;
    uint32_t ipPeriod;
    uint32_t numRefFrames;
    VideoRateControl rcMode;
    VideoRateControlParams rcParams;
    VideoIntraRefreshType refreshType;
    int32_t cyclicFrameInterval;
    AirParams airParams;
    uint32_t disableDeblocking;
    bool syncEncMode;
    int32_t leastInputCount;
}VideoParamsCommon;

typedef struct VideoParamsAVC {
    uint32_t size;
    uint32_t basicUnitSize;     //for rate control
    uint8_t VUIFlag;
    int32_t maxSliceSize;
    uint32_t idrInterval;
    SliceNum sliceNum;
    AVCDelimiterType delimiterType;
    Cropping crop;
    SamplingAspectRatio SAR;
}VideoParamsAVC;

typedef struct VideoParamsUpstreamBuffer {
    uint32_t size;
    VideoBufferSharingMode bufferMode;
    uint32_t *bufList;
    uint32_t bufCnt;
    ExternalBufferAttrib *bufAttrib;
    void *display;
}VideoParamsUpstreamBuffer;

typedef struct VideoParamsUsrptrBuffer {
    uint32_t size;

    //input
    uint32_t width;
    uint32_t height;
    uint32_t format;
    uint32_t expectedSize;

    //output
    uint32_t actualSize;
    uint32_t stride;
    uint8_t *usrPtr;
}VideoParamsUsrptrBuffer;

typedef struct VideoParamsHRD {
    uint32_t size;
    uint32_t bufferSize;
    uint32_t initBufferFullness;
}VideoParamsHRD;

typedef struct VideoParamsStoreMetaDataInBuffers {
    uint32_t size;
    bool isEnabled;
}VideoParamsStoreMetaDataInBuffers;

typedef struct VideoConfigFrameRate {
    uint32_t size;
    VideoFrameRate frameRate;
}VideoConfigFrameRate;

typedef struct VideoConfigBitRate {
    uint32_t size;
    VideoRateControlParams rcParams;
}VideoConfigBitRate;

typedef struct VideoConfigAVCIntraPeriod {
    uint32_t size;
    uint32_t idrInterval;       //How many Intra frame will have a IDR frame
    uint32_t intraPeriod;
}VideoConfigAVCIntraPeriod;

typedef struct VideoConfigNALSize {
    uint32_t size;
    uint32_t maxSliceSize;
}VideoConfigNALSize;

typedef struct VideoConfigResoltuion {
    uint32_t size;
    VideoResolution resolution;
}VideoConfigResoltuion;

typedef struct VideoConfigIntraRefreshType {
    uint32_t size;
    VideoIntraRefreshType refreshType;
}VideoConfigIntraRefreshType;

typedef struct VideoConfigCyclicFrameInterval {
    uint32_t size;
    int32_t cyclicFrameInterval;
}VideoConfigCyclicFrameInterval;

struct VideoConfigAIR {
    uint32_t size;
    AirParams airParams;
};

struct VideoConfigSliceNum {
    uint32_t size;
    SliceNum sliceNum;
};

typedef struct VideoConfigAVCStreamFormat {
    uint32_t size;
    AVCStreamFormat streamFormat;
} VideoConfigAVCStreamFormat;

typedef struct {
    uint32_t total_frames;
    uint32_t skipped_frames;
    uint32_t average_encode_time;
    uint32_t max_encode_time;
    uint32_t max_encode_frame;
    uint32_t min_encode_time;
    uint32_t min_encode_frame;
} VideoStatistics;

#ifdef __cplusplus
}
#endif
#endif                          /*  __VIDEO_ENCODER_DEF_H__ */
