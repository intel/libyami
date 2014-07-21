/*
 *  VideoEncoderDef.h- basic va decoder for video
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

#include <va/va.h>
#include <stdint.h>

namespace YamiMediaCodec{
#define STRING_TO_FOURCC(format) ((uint32_t)(((format)[0])|((format)[1]<<8)|((format)[2]<<16)|((format)[3]<<24)))
#define min(X,Y) (((X) < (Y)) ? (X) : (Y))
#define max(X,Y) (((X) > (Y)) ? (X) : (Y))

typedef int32_t Encode_Status;

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
};

enum VideoOutputFormat {
    OUTPUT_EVERYTHING = 0,      //Output whatever driver generates
    OUTPUT_CODEC_DATA = 1,
    OUTPUT_FRAME_DATA = 2,      //Equal to OUTPUT_EVERYTHING when no header along with the frame data
    OUTPUT_ONE_NAL = 4,
    OUTPUT_ONE_NAL_WITHOUT_STARTCODE = 8,
    OUTPUT_LENGTH_PREFIXED = 16,
    OUTPUT_BUFFER_LAST
};

enum VideoRawFormat {
    RAW_FORMAT_NONE = 0,
    RAW_FORMAT_YUV420 = 1,
    RAW_FORMAT_YUV422 = 2,
    RAW_FORMAT_YUV444 = 4,
    RAW_FORMAT_NV12 = 8,
    RAW_FORMAT_PROTECTED = 0x80000000,
    RAW_FORMAT_LAST
};

enum VideoRateControl {
    RATE_CONTROL_NONE = 1,
    RATE_CONTROL_CBR = 2,
    RATE_CONTROL_VBR = 4,
    RATE_CONTROL_VCM = 8,
    RATE_CONTROL_CQP = 16,
    RATE_CONTROL_LAST
};

enum VideoProfile {
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
};

enum AVCDelimiterType {
    AVC_DELIMITER_LENGTHPREFIX = 0,
    AVC_DELIMITER_ANNEXB
};

enum VideoIntraRefreshType {
    VIDEO_ENC_NONIR,            // Non intra refresh
    VIDEO_ENC_CIR,              // Cyclic intra refresh
    VIDEO_ENC_AIR,              // Adaptive intra refresh
    VIDEO_ENC_BOTH,
    VIDEO_ENC_LAST
};

enum VideoBufferSharingMode {
    BUFFER_SHARING_NONE = 1,    //Means non shared buffer mode
    BUFFER_SHARING_CI = 2,
    BUFFER_SHARING_V4L2 = 4,
    BUFFER_SHARING_SURFACE = 8,
    BUFFER_SHARING_USRPTR = 16,
    BUFFER_SHARING_GFXHANDLE = 32,
    BUFFER_SHARING_KBUFHANDLE = 64,
    BUFFER_LAST
};

// Output buffer flag
#define ENCODE_BUFFERFLAG_ENDOFFRAME       0x00000001
#define ENCODE_BUFFERFLAG_PARTIALFRAME     0x00000002
#define ENCODE_BUFFERFLAG_SYNCFRAME        0x00000004
#define ENCODE_BUFFERFLAG_CODECCONFIG      0x00000008
#define ENCODE_BUFFERFLAG_DATACORRUPT      0x00000010
#define ENCODE_BUFFERFLAG_DATAINVALID      0x00000020
#define ENCODE_BUFFERFLAG_SLICEOVERFOLOW   0x00000040

struct VideoEncOutputBuffer {
    uint8_t *data;
    uint32_t bufferSize;        //buffer size
    uint32_t dataSize;          //actuall size
    uint32_t remainingSize;
    int flag;                   //Key frame, Codec Data etc
    VideoOutputFormat format;   //output format
    uint64_t timeStamp;         //reserved

     VideoEncOutputBuffer():data(0), bufferSize(0), dataSize(0)
    , remainingSize(0), flag(0), format(OUTPUT_BUFFER_LAST), timeStamp(0) {
    };
};

struct VideoEncRawBuffer {
    uint8_t *data;
    uint32_t size;
    bool bufAvailable;          //To indicate whether this buffer can be reused
    uint64_t timeStamp;         //reserved
    bool forceKeyFrame;

     VideoEncRawBuffer():data(0), size(0), bufAvailable(false),
        timeStamp(0), forceKeyFrame(false) {
}};

struct VideoEncSurfaceBuffer {
    VASurfaceID surface;
    uint8_t *usrptr;
    uint32_t index;
    bool bufAvailable;
    VideoEncSurfaceBuffer *next;
     VideoEncSurfaceBuffer():surface(VA_INVALID_ID), usrptr(0)
    , index(0), bufAvailable(false) {
}};

struct AirParams {
    uint32_t airMBs;
    uint32_t airThreshold;
    uint32_t airAuto;

     AirParams():airMBs(0), airThreshold(0), airAuto(0) {
    };

    AirParams & operator=(const AirParams & other) {
        if (this == &other)
            return *this;

        this->airMBs = other.airMBs;
        this->airThreshold = other.airThreshold;
        this->airAuto = other.airAuto;
        return *this;
    }
};

struct VideoFrameRate {
    uint32_t frameRateNum;
    uint32_t frameRateDenom;

     VideoFrameRate():frameRateNum(0), frameRateDenom(1) {
    } VideoFrameRate & operator=(const VideoFrameRate & other) {
        if (this == &other)
            return *this;

        this->frameRateNum = other.frameRateNum;
        this->frameRateDenom = other.frameRateDenom;
        return *this;
    }
};

struct VideoResolution {
    uint32_t width;
    uint32_t height;

     VideoResolution():width(0), height(0) {
    };

    VideoResolution & operator=(const VideoResolution & other) {
        if (this == &other)
            return *this;

        this->width = other.width;
        this->height = other.height;
        return *this;
    }
};

struct VideoRateControlParams {
    uint32_t bitRate;
    uint32_t initQP;
    uint32_t minQP;
    uint32_t windowSize;
    uint32_t targetPercentage;
    uint32_t disableFrameSkip;
    uint32_t disableBitsStuffing;

     VideoRateControlParams():bitRate(0), initQP(0), minQP(0)
    , windowSize(0), targetPercentage(0)
    , disableFrameSkip(0), disableBitsStuffing(0) {
    };

    VideoRateControlParams & operator=(const VideoRateControlParams &
                                       other) {
        if (this == &other)
            return *this;

        this->bitRate = other.bitRate;
        this->initQP = other.initQP;
        this->minQP = other.minQP;
        this->windowSize = other.windowSize;
        this->targetPercentage = other.targetPercentage;
        this->disableFrameSkip = other.disableFrameSkip;
        this->disableBitsStuffing = other.disableBitsStuffing;
        return *this;
    };
};

struct SliceNum {
    uint32_t iSliceNum;
    uint32_t pSliceNum;

     SliceNum():iSliceNum(1), pSliceNum(1) {
    };

    SliceNum & operator=(const SliceNum & other) {
        if (this == &other)
            return *this;

        this->iSliceNum = other.iSliceNum;
        this->pSliceNum = other.pSliceNum;
        return *this;
    };
};

typedef struct {
    uint32_t realWidth;
    uint32_t realHeight;
    uint32_t lumaStride;
    uint32_t chromStride;
    uint32_t format;
} ExternalBufferAttrib;

struct Cropping {
    uint32_t LeftOffset;
    uint32_t RightOffset;
    uint32_t TopOffset;
    uint32_t BottomOffset;

     Cropping():LeftOffset(0), RightOffset(0),
        TopOffset(0), BottomOffset(0) {
    };

    Cropping & operator=(const Cropping & other) {
        if (this == &other)
            return *this;

        this->LeftOffset = other.LeftOffset;
        this->RightOffset = other.RightOffset;
        this->TopOffset = other.TopOffset;
        this->BottomOffset = other.BottomOffset;
        return *this;
    }
};

struct SamplingAspectRatio {
    uint16_t SarWidth;
    uint16_t SarHeight;

     SamplingAspectRatio():SarWidth(0), SarHeight(0) {
    };

    SamplingAspectRatio & operator=(const SamplingAspectRatio & other) {
        if (this == &other)
            return *this;
        this->SarWidth = other.SarWidth;
        this->SarHeight = other.SarHeight;
        return *this;
    }
};

enum VideoParamConfigType {
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

    VideoParamsConfigExtension
};

struct VideoParamConfigSet {
    VideoParamConfigType type;
    uint32_t size;

  protected:
    VideoParamConfigSet(VideoParamConfigType t, uint32_t s)
    :type(t), size(s) {
    };

    VideoParamConfigSet & operator=(const VideoParamConfigSet & other) {
        if (this == &other)
            return *this;
        this->type = other.type;
        this->size = other.size;
        return *this;
    }

  private:
    VideoParamConfigSet() {
    };
};

struct VideoParamsCommon:VideoParamConfigSet {

    VAProfile profile;
    uint8_t level;
    VideoRawFormat rawFormat;
    VideoResolution resolution;
    VideoFrameRate frameRate;
    int32_t intraPeriod;
    VideoRateControl rcMode;
    VideoRateControlParams rcParams;
    VideoIntraRefreshType refreshType;
    int32_t cyclicFrameInterval;
    AirParams airParams;
    uint32_t disableDeblocking;
    bool syncEncMode;

    VideoParamsCommon()
    :VideoParamConfigSet(VideoParamsTypeCommon, sizeof(VideoParamsCommon))
    , profile(VAProfileNone)
    , level(0)
    , rawFormat(RAW_FORMAT_NONE)
    , resolution()
    , intraPeriod(0)
    , rcMode(RATE_CONTROL_NONE)
    , rcParams()
    , refreshType(VIDEO_ENC_NONIR)
    , cyclicFrameInterval(0)
    , airParams()
    , disableDeblocking(0)
    , syncEncMode(true) {
    };

    VideoParamsCommon & operator=(const VideoParamsCommon & other) {
        if (this == &other)
            return *this;

        VideoParamConfigSet::operator=(other);
        this->profile = other.profile;
        this->level = other.level;
        this->rawFormat = other.rawFormat;
        this->resolution = other.resolution;
        this->frameRate = other.frameRate;
        this->intraPeriod = other.intraPeriod;
        this->rcMode = other.rcMode;
        this->rcParams = other.rcParams;
        this->refreshType = other.refreshType;
        this->cyclicFrameInterval = other.cyclicFrameInterval;
        this->airParams = other.airParams;
        this->disableDeblocking = other.disableDeblocking;
        this->syncEncMode = other.syncEncMode;
        return *this;
    }
};

struct VideoParamsAVC:VideoParamConfigSet {

    uint32_t basicUnitSize;     //for rate control
    uint8_t VUIFlag;
    int32_t maxSliceSize;
    uint32_t idrInterval;
    SliceNum sliceNum;
    AVCDelimiterType delimiterType;
    Cropping crop;
    SamplingAspectRatio SAR;

     VideoParamsAVC()
    :VideoParamConfigSet(VideoParamsTypeAVC, sizeof(VideoParamsAVC))
    , basicUnitSize(0)
    , VUIFlag(0)
    , maxSliceSize(0)
    , idrInterval(0)
    , sliceNum()
    , delimiterType(AVC_DELIMITER_ANNEXB)
    , crop()
    , SAR() {
    };

    VideoParamsAVC & operator=(const VideoParamsAVC & other) {
        if (this == &other)
            return *this;

        VideoParamConfigSet::operator=(other);
        this->basicUnitSize = other.basicUnitSize;
        this->VUIFlag = other.VUIFlag;
        this->maxSliceSize = other.maxSliceSize;
        this->idrInterval = other.idrInterval;
        this->sliceNum = other.sliceNum;
        this->delimiterType = other.delimiterType;
        this->crop.LeftOffset = other.crop.LeftOffset;
        this->crop.RightOffset = other.crop.RightOffset;
        this->crop.TopOffset = other.crop.TopOffset;
        this->crop.BottomOffset = other.crop.BottomOffset;
        this->SAR.SarWidth = other.SAR.SarWidth;
        this->SAR.SarHeight = other.SAR.SarHeight;

        return *this;
    }
};

struct VideoParamsUpstreamBuffer:VideoParamConfigSet {

    VideoParamsUpstreamBuffer()
    :VideoParamConfigSet(VideoParamsTypeUpSteamBuffer,
                         sizeof(VideoParamsUpstreamBuffer))
    , bufferMode(BUFFER_SHARING_NONE)
    , bufList(0)
    , bufCnt(0)
    , bufAttrib(0)
    , display(0) {
    };

    VideoBufferSharingMode bufferMode;
    uint32_t *bufList;
    uint32_t bufCnt;
    ExternalBufferAttrib *bufAttrib;
    void *display;
};

struct VideoParamsUsrptrBuffer:VideoParamConfigSet {

    VideoParamsUsrptrBuffer()
    :VideoParamConfigSet(VideoParamsTypeUsrptrBuffer,
                         sizeof(VideoParamsUsrptrBuffer))
    , width(0), height(0), format(0), expectedSize(0) {
    }

    //input
    uint32_t width;
    uint32_t height;
    uint32_t format;
    uint32_t expectedSize;

    //output
    uint32_t actualSize;
    uint32_t stride;
    uint8_t *usrPtr;
};

struct VideoParamsHRD:VideoParamConfigSet {

    VideoParamsHRD():VideoParamConfigSet(VideoParamsTypeHRD,
                                         sizeof(VideoParamsHRD))
    , bufferSize(0), initBufferFullness(0) {
    };

    uint32_t bufferSize;
    uint32_t initBufferFullness;
};

struct VideoParamsStoreMetaDataInBuffers:VideoParamConfigSet {

    VideoParamsStoreMetaDataInBuffers()
    :VideoParamConfigSet(VideoParamsTypeStoreMetaDataInBuffers,
                         sizeof(VideoParamsStoreMetaDataInBuffers))
    , isEnabled(false) {
    }
    bool isEnabled;
};

struct VideoConfigFrameRate:VideoParamConfigSet {

    VideoConfigFrameRate()
    :VideoParamConfigSet(VideoConfigTypeFrameRate,
                         sizeof(VideoConfigFrameRate))
    , frameRate() {
    }
    VideoFrameRate frameRate;
};

struct VideoConfigBitRate:VideoParamConfigSet {

    VideoConfigBitRate()
    :VideoParamConfigSet(VideoConfigTypeBitRate,
                         sizeof(VideoConfigBitRate))
    , rcParams() {
    }
    VideoRateControlParams rcParams;
};

struct VideoConfigAVCIntraPeriod:VideoParamConfigSet {

    VideoConfigAVCIntraPeriod()
    :VideoParamConfigSet(VideoConfigTypeAVCIntraPeriod,
                         sizeof(VideoConfigAVCIntraPeriod))
    , idrInterval(0), intraPeriod(0) {
    }
    uint32_t idrInterval;       //How many Intra frame will have a IDR frame
    uint32_t intraPeriod;
};

struct VideoConfigNALSize:VideoParamConfigSet {

    VideoConfigNALSize():VideoParamConfigSet(VideoConfigTypeNALSize,
                                             sizeof(VideoConfigNALSize))
    , maxSliceSize(1) {
    }
    uint32_t maxSliceSize;
};

struct VideoConfigResoltuion:VideoParamConfigSet {

    VideoConfigResoltuion()
    :VideoParamConfigSet(VideoConfigTypeResolution,
                         sizeof(VideoConfigResoltuion))
    , resolution() {
    }
    VideoResolution resolution;
};

struct VideoConfigIntraRefreshType:VideoParamConfigSet {

    VideoConfigIntraRefreshType()
    :VideoParamConfigSet(VideoConfigTypeIntraRefreshType,
                         sizeof(VideoConfigIntraRefreshType))
    , refreshType(VIDEO_ENC_NONIR) {
    }
    VideoIntraRefreshType refreshType;
};

struct VideoConfigCyclicFrameInterval:VideoParamConfigSet {

    VideoConfigCyclicFrameInterval()
    :VideoParamConfigSet(VideoConfigTypeCyclicFrameInterval,
                         sizeof(VideoConfigCyclicFrameInterval))
    , cyclicFrameInterval(0) {
    }
    int32_t cyclicFrameInterval;
};

struct VideoConfigAIR:VideoParamConfigSet {

    VideoConfigAIR():VideoParamConfigSet(VideoConfigTypeAIR,
                                         sizeof(VideoConfigAIR))
    , airParams() {
    }
    AirParams airParams;
};

struct VideoConfigSliceNum:VideoParamConfigSet {

    VideoConfigSliceNum():VideoParamConfigSet(VideoConfigTypeSliceNum,
                                              sizeof(VideoConfigSliceNum))
    , sliceNum() {
    }
    SliceNum sliceNum;
};

typedef struct {
    uint32_t total_frames;
    uint32_t skipped_frames;
    uint32_t average_encode_time;
    uint32_t max_encode_time;
    uint32_t max_encode_frame;
    uint32_t min_encode_time;
    uint32_t min_encode_frame;
} VideoStatistics;
}
#endif                          /*  __VIDEO_ENCODER_DEF_H__ */
