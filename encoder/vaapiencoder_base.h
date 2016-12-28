/*
 * Copyright (C) 2013-2014 Intel Corporation. All rights reserved.
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

#ifndef vaapiencoder_base_h
#define vaapiencoder_base_h

#include "VideoEncoderDefs.h"
#include "VideoEncoderInterface.h"
#include "common/lock.h"
#include "common/log.h"
#include "common/surfacepool.h"
#include "vaapiencpicture.h"
#include "vaapi/VaapiBuffer.h"
#include "vaapi/vaapiptrs.h"
#include "vaapi/VaapiSurface.h"

#include <deque>
#include <utility>

template <class B, class C> class FactoryTest;

namespace YamiMediaCodec{
enum VaapiEncReorderState
{
    VAAPI_ENC_REORD_NONE = 0,
    VAAPI_ENC_REORD_DUMP_FRAMES = 1,
    VAAPI_ENC_REORD_WAIT_FRAMES = 2
};

class VaapiEncoderBase : public IVideoEncoder {
    typedef SharedPtr<VaapiEncPicture> PicturePtr;
public:
    VaapiEncoderBase();
    virtual ~VaapiEncoderBase();

    virtual void  setNativeDisplay(NativeDisplay * nativeDisplay);
    virtual YamiStatus start(void) = 0;
    virtual void flush(void) = 0;
    virtual YamiStatus stop(void) = 0;
    virtual YamiStatus encode(VideoEncRawBuffer* inBuffer);
    virtual YamiStatus encode(VideoFrameRawData* frame);
    virtual YamiStatus encode(const SharedPtr<VideoFrame>& frame);

/*
    * getOutput can be called several time for a frame (such as first time  codec data, and second time others)
    * encode will provide encoded data according to the format (whole frame, codec_data, sigle NAL etc)
    * If the buffer passed to encoded is not big enough, this API call will return YAMI_ENCODE_BUFFER_TOO_SMALL
    * and caller should provide a big enough buffer and call again
    */
#ifndef __BUILD_GET_MV__
    virtual YamiStatus getOutput(VideoEncOutputBuffer* outBuffer, bool withWait = false);
#else
    virtual YamiStatus getOutput(VideoEncOutputBuffer* outBuffer, VideoEncMVBuffer* MVBuffer, bool withWait = false);
#endif
    virtual YamiStatus getParameters(VideoParamConfigType type, Yami_PTR);
    virtual YamiStatus setParameters(VideoParamConfigType type, Yami_PTR);
    virtual YamiStatus setConfig(VideoParamConfigType type, Yami_PTR);
    virtual YamiStatus getConfig(VideoParamConfigType type, Yami_PTR);

    virtual YamiStatus getMaxOutSize(uint32_t* maxSize);

#ifdef __BUILD_GET_MV__
    /// get MV buffer size.
    virtual YamiStatus getMVBufferSize(uint32_t* Size);
#endif
    virtual void getPicture(PicturePtr &outPicture);
    virtual YamiStatus checkCodecData(VideoEncOutputBuffer* outBuffer);
    virtual YamiStatus checkEmpty(VideoEncOutputBuffer* outBuffer, bool* outEmpty);
    virtual YamiStatus getStatistics(VideoStatistics* videoStat)
    {
        return YAMI_SUCCESS;
    };

protected:
    //utils functions for derived class
    SurfacePtr createNewSurface(uint32_t fourcc);
    SurfacePtr createSurface();
    SurfacePtr createSurface(VideoFrameRawData* frame);
    SurfacePtr createSurface(const SharedPtr<VideoFrame>& frame);

    template <class Pic>
    bool output(const SharedPtr<Pic>&);
    virtual YamiStatus getCodecConfig(VideoEncOutputBuffer* outBuffer);

    //virtual functions
    virtual YamiStatus doEncode(const SurfacePtr&, uint64_t timeStamp, bool forceKeyFrame = false) = 0;

    //rate control related things
    void fill(VAEncMiscParameterHRD*) const ;
    void fill(VAEncMiscParameterRateControl*) const ;
    void fill(VAEncMiscParameterFrameRate*) const;
    virtual bool ensureMiscParams(VaapiEncPicture*);

    //properties
    VideoProfile profile() const;
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
    uint32_t ipPeriod() const {
        return m_videoParamCommon.ipPeriod;
    }

    uint32_t numRefFrames() const {
        return m_videoParamCommon.numRefFrames;
    }

    uint32_t frameRateDenom() const {
        return m_videoParamCommon.frameRate.frameRateDenom;
    }
    uint32_t frameRateNum() const {
        return m_videoParamCommon.frameRate.frameRateNum;
    }

    uint32_t fps() const {
        return m_videoParamCommon.frameRate.frameRateNum / m_videoParamCommon.frameRate.frameRateDenom;
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

    uint32_t& initQP() { return m_videoParamCommon.rcParams.initQP; }

    uint32_t minQP() const {
        return m_videoParamCommon.rcParams.minQP;
    }

    uint32_t& minQP() {
        return m_videoParamCommon.rcParams.minQP;
    }

    uint32_t maxQP() const {
        return m_videoParamCommon.rcParams.maxQP;
    }

    bool isBusy();

    bool mapToRange(uint32_t& value,
        uint32_t min, uint32_t max,
        uint32_t level,
        uint32_t minLevel, uint32_t maxLevel);
    bool mapQualityLevel();
    bool fillQualityLevel(VaapiEncPicture*);

    DisplayPtr m_display;
    ContextPtr m_context;
    VAEntrypoint m_entrypoint;
    VideoParamsCommon m_videoParamCommon;
    VideoParamsHRD m_videoParamsHRD;
    bool m_videoParamQualityLevelUpdate;
    VideoParamsQualityLevel m_videoParamQualityLevel;
    uint32_t m_vaVideoParamQualityLevel;
    uint32_t m_maxOutputBuffer; // max count of frames are encoding in parallel, it hurts performance when m_maxOutputBuffer is too big.
    uint32_t m_maxCodedbufSize;

private:
    bool initVA();
    void cleanupVA();
    NativeDisplay m_externalDisplay;

    SharedPtr<SurfacePool> m_pool;
    SharedPtr<SurfaceAllocator> m_alloc;

    Lock m_lock;
    typedef std::deque<PicturePtr> OutputQueue;
    OutputQueue m_output;

    bool updateMaxOutputBufferCount() {
        if (m_maxOutputBuffer < m_videoParamCommon.leastInputCount + 3)
            m_maxOutputBuffer = m_videoParamCommon.leastInputCount + 3;
        return true;
    }
};

template <class Pic>
bool VaapiEncoderBase::output(const SharedPtr<Pic>& pic)
{
    bool ret;
    PicturePtr picture;
    AutoLock l(m_lock);
    picture = DynamicPointerCast<VaapiEncPicture>(pic);
    if (picture) {
        m_output.push_back(picture);
        ret = true;
    } else {
        ERROR("output need a subclass of VaapiEncPicutre");
        ret = false;
    }
    return ret;
}

}
#endif /* vaapiencoder_base_h */
