/*
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __VIDEO_ENCODER_DEF_H__
#define __VIDEO_ENCODER_DEF_H__

// config.h should NOT be included in header file, especially for the header file used by external

#include <va/va.h>
#include <stdint.h>
#include <VideoCommonDefs.h>

#define STRING_TO_FOURCC(format) ((uint32_t)(((format)[0])|((format)[1]<<8)|((format)[2]<<16)|((format)[3]<<24)))

#ifdef __cplusplus
extern "C" {
#endif

typedef void * Yami_PTR;

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
    RATE_CONTROL_NONE = VA_RC_NONE,
    RATE_CONTROL_CBR = VA_RC_CBR,
    RATE_CONTROL_VBR = VA_RC_VBR,
    RATE_CONTROL_VCM = VA_RC_VCM,
    RATE_CONTROL_CQP = VA_RC_CQP,
    RATE_CONTROL_LAST
}VideoRateControl;

typedef enum {
    PROFILE_INVALID,
    PROFILE_JPEG_BASELINE,
    PROFILE_VC1_SIMPLE,
    PROFILE_VC1_MAIN,
    PROFILE_VC1_ADVANCED,
    PROFILE_MPEG2_SIMPLE,
    PROFILE_MPEG2_MAIN,
    PROFILE_MPEG4_SIMPLE,
    PROFILE_MPEG4_ADVANCED_SIMPLE,
    PROFILE_MPEG4_MAIN,
    PROFILE_H263_BASELINE,
    PROFILE_H264_CONSTRAINED_BASELINE,
    PROFILE_H264_MAIN,
    PROFILE_H264_HIGH,
    PROFILE_H264_HIGH10,
    PROFILE_H264_HIGH422,
    PROFILE_H264_HIGH444,
    PROFILE_H265_MAIN,
    PROFILE_H265_MAIN10,
} VideoProfile;

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

//quality level for encoder
#define VIDEO_PARAMS_QUALITYLEVEL_NONE 0
#define VIDEO_PARAMS_QUALITYLEVEL_MIN 1
#define VIDEO_PARAMS_QUALITYLEVEL_MAX 7
#define TEMPORAL_LAYER_LENGTH_MAX 32

typedef struct VideoEncOutputBuffer {
    uint8_t *data;
    uint32_t bufferSize;        //buffer size
    uint32_t dataSize;          //actuall size
    uint32_t remainingSize;
    uint32_t flag;                   //Key frame, Codec Data etc
    VideoOutputFormat format;   //output format
    uint8_t temporalID;			//output temporal id for SVCT encoder
    uint64_t timeStamp;         //reserved
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
}VideoEncRawBuffer;

typedef struct VideoEncSurfaceBuffer {
    VASurfaceID surface;
    uint8_t *usrptr;
    uint32_t index;
    bool bufAvailable;
    struct EncSurfaceBuffer *next;
}VideoEncSurfaceBuffer;

typedef struct VideoFrameRate {
    uint32_t frameRateNum;
    uint32_t frameRateDenom;
}VideoFrameRate;
typedef struct VideoTemporalLayers {
    //layer 0 bitrate is bitRate[0]
    //...
    //layer highest birate is VideoParamsCommon.rcParams.bitRate
    uint8_t numLayersMinus1;
    uint32_t bitRate[TEMPORAL_LAYER_LENGTH_MAX];
} VideoTemporalLayers;

typedef struct VideoResolution {
    uint32_t width;
    uint32_t height;
}VideoResolution;

typedef struct VideoRateControlParams {
    uint32_t bitRate;
    uint32_t initQP;
    uint32_t minQP;
    uint32_t maxQP;
    uint32_t disableFrameSkip;
    uint32_t disableBitsStuffing;
    int8_t diffQPIP;// P frame qp minus initQP
    int8_t diffQPIB;// B frame qp minus initQP
}VideoRateControlParams;

typedef struct SliceNum {
    uint32_t iSliceNum;
    uint32_t pSliceNum;
}SliceNum;

typedef struct SamplingAspectRatio {
    uint16_t SarWidth;
    uint16_t SarHeight;
}SamplingAspectRatio;

typedef enum {
    VideoParamsTypeStartUnused = 0x01000000,
    VideoParamsTypeCommon,
    VideoParamsTypeAVC,
    VideoParamsTypeH263,
    VideoParamsTypeMP4,
    VideoParamsTypeVP9,
    VideoParamsTypeVC1,
    VideoParamsTypeHRD,
    VideoParamsTypeQualityLevel,

    VideoConfigTypeFrameRate,
    VideoConfigTypeBitRate,
    VideoConfigTypeResolution,
    VideoConfigTypeNALSize,
    VideoConfigTypeIDRRequest,
    VideoConfigTypeSliceNum,

    //format related
    VideoConfigTypeAVCStreamFormat,

    VideoParamsConfigExtension
} VideoParamConfigType;

typedef struct VideoParamConfigSet {
    uint32_t size;
}VideoParamConfigSet;

typedef struct VideoParamsCommon {
    uint32_t size;
    VAProfile profile;
    uint8_t level;
    VideoResolution resolution;
    VideoFrameRate frameRate;
    VideoTemporalLayers temporalLayers;
    uint32_t intraPeriod;
    uint32_t ipPeriod;
    uint32_t numRefFrames;
    VideoRateControl rcMode;
    VideoRateControlParams rcParams;
    uint32_t leastInputCount;
    bool enableLowPower;
    uint8_t bitDepth;
}VideoParamsCommon;

typedef struct VideoParamsAVC {
    uint32_t size;
    uint32_t basicUnitSize;     //for rate control
    uint8_t VUIFlag;
    int32_t maxSliceSize;
    uint32_t idrInterval;    //How many Intra frames will have an IDR frame
    SliceNum sliceNum;
    SamplingAspectRatio SAR;
    bool  enableCabac;
    bool  enableDct8x8;
    bool  enableDeblockFilter;
    int8_t deblockAlphaOffsetDiv2; //same as slice_alpha_c0_offset_div2 defined in h264 spec 7.4.3
    int8_t deblockBetaOffsetDiv2; //same as slice_beta_offset_div2 defined in h264 spec 7.4.3
    uint32_t priorityId;
    // enable prefix NAL unit defined as h264 spec G.3.42.
    // It can be used for h264 simucast or svc-t encoding.
    bool enablePrefixNalUnit;
}VideoParamsAVC;

typedef struct VideoParamsVP9 {
    uint32_t referenceMode;
}VideoParamsVP9;

typedef struct VideoParamsHRD {
    uint32_t size;
    uint32_t bufferSize;
    uint32_t initBufferFullness;
    uint32_t windowSize; // use for HRD CPB length in ms
    uint32_t targetPercentage;
}VideoParamsHRD;

typedef struct VideoParamsQualityLevel {
    uint32_t size;
    uint32_t level;
} VideoParamsQualityLevel;

typedef struct VideoConfigFrameRate {
    uint32_t size;
    VideoFrameRate frameRate;
}VideoConfigFrameRate;

typedef struct VideoConfigBitRate {
    uint32_t size;
    VideoRateControlParams rcParams;
}VideoConfigBitRate;

typedef struct VideoConfigNALSize {
    uint32_t size;
    uint32_t maxSliceSize;
}VideoConfigNALSize;

typedef struct VideoConfigResoltuion {
    uint32_t size;
    VideoResolution resolution;
}VideoConfigResoltuion;

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
