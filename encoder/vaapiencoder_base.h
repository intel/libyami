/*
 *  vaapiencoder_h264.cpp - h264 encoder for va
 *
 *  Copyright (C) 2013-2014 Intel Corporation
 *    Author: Wind Yuan <feng.yuan@intel.com>
 *    Author: Xu Guangxin <guangxin.xu@intel.com>
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

#ifndef vaapiencoder_base_h
#define vaapiencoder_base_h

#include "interface/VideoEncoderDef.h"
#include "interface/VideoEncoderInterface.h"
#include "common/log.h"
#include "vaapiencpicture.h"
#include "vaapi/vaapibuffer.h"
#include "vaapi/vaapiptrs.h"
#include "vaapi/vaapisurface.h"

namespace YamiMediaCodec{
enum VaapiEncReorderState
{
    VAAPI_ENC_REORD_NONE = 0,
    VAAPI_ENC_REORD_DUMP_FRAMES = 1,
    VAAPI_ENC_REORD_WAIT_FRAMES = 2
};

class VaapiEncoderBase : public IVideoEncoder {
public:
    VaapiEncoderBase();
    virtual ~VaapiEncoderBase();

    virtual void  setXDisplay(Display * xdisplay);
    virtual Encode_Status start(void) = 0;
    virtual void flush(void) = 0;
    virtual Encode_Status stop(void) = 0;
    virtual Encode_Status encode(VideoEncRawBuffer *inBuffer);

    /*
    * getOutput can be called several time for a frame (such as first time  codec data, and second time others)
    * encode will provide encoded data according to the format (whole frame, codec_data, sigle NAL etc)
    * If the buffer passed to encoded is not big enough, this API call will return ENCODE_BUFFER_TOO_SMALL
    * and caller should provide a big enough buffer and call again
    */
    virtual Encode_Status getOutput(VideoEncOutputBuffer * outBuffer, bool withWait = false) const = 0;

    virtual Encode_Status getParameters(VideoParamConfigSet *);
    virtual Encode_Status setParameters(VideoParamConfigSet *);
    virtual Encode_Status setConfig(VideoParamConfigSet *);
    virtual Encode_Status getConfig(VideoParamConfigSet *);

    virtual Encode_Status getMaxOutSize(uint32_t *maxSize);

    virtual Encode_Status getStatistics(VideoStatistics *videoStat) {
        return ENCODE_SUCCESS;
    };

protected:
    //utils functions for derived class
    SurfacePtr createSurface();
    SurfacePtr createSurface(VideoEncRawBuffer* inBuffer);
    Encode_Status copyCodedBuffer(VideoEncOutputBuffer *, const CodedBufferPtr&) const;

    //virtual functions
    virtual Encode_Status reorder(const SurfacePtr& , uint64_t timeStamp, bool forceKeyFrame = false) = 0;
    virtual Encode_Status submitEncode() = 0;

    //rate control related things
    void fill(VAEncMiscParameterHRD*) const ;
    void fill(VAEncMiscParameterRateControl*) const ;
    bool ensureMiscParams (VaapiEncPicture*);

    //properties
    VaapiProfile profile() const;
    uint8_t level () const {
        return m_videoParamCommon.level;
    }
    uint32_t width() const {
        return m_videoParamCommon.resolution.width;
    }
    uint32_t height() const {
        return m_videoParamCommon.resolution.height;
    }
    uint32_t intraPeriod() const {
        return m_videoParamCommon.intraPeriod;
    }
    uint32_t frameRateDenom() const {
        return m_videoParamCommon.frameRate.frameRateDenom;
    }
    uint32_t frameRateNum() const {
        return m_videoParamCommon.frameRate.frameRateNum;
    }
    //rate control
    VideoRateControl rateControlMode() const {
        return m_videoParamCommon.rcMode;
    }
    uint32_t bitRate() const {
        return m_videoParamCommon.rcParams.bitRate;
    }
    uint32_t initQP() const {
        return m_videoParamCommon.rcParams.initQP;
    }

    uint32_t minQP() const {
        return m_videoParamCommon.rcParams.minQP;
    }

    uint32_t& minQP() {
        return m_videoParamCommon.rcParams.minQP;
    }
    virtual bool isBusy() = 0 ;

    DisplayPtr m_display;
    ContextPtr m_context;
    VAEntrypoint m_entrypoint;
    VideoParamsCommon m_videoParamCommon;
    uint32_t m_maxOutputBuffer; // max count of frames are encoding in parallel, it hurts performance when m_maxOutputBuffer is too big.
    uint32_t m_maxCodedbufSize;

private:
    bool initVA();
    void cleanupVA();
    Display* m_externalDisplay;

    bool updateMaxOutputBufferCount() {
        if (m_maxOutputBuffer < m_videoParamCommon.leastInputCount + 3)
            m_maxOutputBuffer = m_videoParamCommon.leastInputCount + 3;
    }
};
}
#endif /* vaapiencoder_base_h */
