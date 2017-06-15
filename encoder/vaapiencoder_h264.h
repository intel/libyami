/*
 * Copyright (C) 2013-2016 Intel Corporation. All rights reserved.
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

#ifndef vaapiencoder_h264_h
#define vaapiencoder_h264_h

#include "vaapiencoder_base.h"
#include "vaapi/vaapiptrs.h"
#include "common/lock.h"
#include <list>
#include <queue>
#include <deque>
#include <pthread.h>
#include <va/va_enc_h264.h>

namespace YamiMediaCodec{
class VaapiEncPictureH264;
class VaapiEncoderH264Ref;
class VaapiEncStreamHeaderH264;

class VaapiEncoderH264 : public VaapiEncoderBase {
public:
    //shortcuts, It's intended to elimilate codec diffrence
    //to make template for other codec implelmentation.
    typedef SharedPtr<VaapiEncPictureH264> PicturePtr;
    typedef SharedPtr<VaapiEncoderH264Ref> ReferencePtr;
    typedef SharedPtr <VaapiEncStreamHeaderH264> StreamHeaderPtr;

    VaapiEncoderH264();
    ~VaapiEncoderH264();
    virtual YamiStatus start();
    virtual void flush();
    virtual YamiStatus stop();

    virtual YamiStatus getParameters(VideoParamConfigType type, Yami_PTR);
    virtual YamiStatus setParameters(VideoParamConfigType type, Yami_PTR);
    virtual YamiStatus getMaxOutSize(uint32_t* maxSize);
#ifdef __BUILD_GET_MV__
    // get MV buffer size.
    virtual YamiStatus getMVBufferSize(uint32_t* Size);
#endif

protected:
    virtual YamiStatus doEncode(const SurfacePtr&, uint64_t timeStamp, bool forceKeyFrame);
    virtual YamiStatus getCodecConfig(VideoEncOutputBuffer* outBuffer);
    virtual bool ensureMiscParams(VaapiEncPicture*);

private:
    friend class FactoryTest<IVideoEncoder, VaapiEncoderH264>;
    friend class VaapiEncoderH264Test;

    //following code is a template for other encoder implementation
    YamiStatus encodePicture(const PicturePtr&);

#if VA_CHECK_VERSION(0, 39, 4)
    void fill(VAEncMiscParameterTemporalLayerStructure*) const;
#endif
    bool fill(VAEncSequenceParameterBufferH264*) const;
    bool fill(VAEncPictureParameterBufferH264*, const PicturePtr&, const SurfacePtr&) const ;
    bool ensureSequenceHeader(const PicturePtr&, const VAEncSequenceParameterBufferH264* const);
    bool ensurePictureHeader(const PicturePtr&, const VAEncPictureParameterBufferH264* const );
    bool addSliceHeaders (const PicturePtr&) const;
    bool ensureSequence(const PicturePtr&);
    bool ensurePicture (const PicturePtr&, const SurfacePtr&);
    bool ensureSlices(const PicturePtr&);
    bool ensureCodedBufferSize();
    bool addPackedPrefixNalUnit(const PicturePtr&) const;
    bool addPackedSliceHeader(
        const PicturePtr& picture,
        const VAEncSliceParameterBufferH264* const sliceParam) const;

    //reference list related
    YamiStatus reorder(const SurfacePtr& surface, uint64_t timeStamp, bool forceKeyFrame);
    bool fillReferenceList(VAEncSliceParameterBufferH264* slice) const;
    bool referenceListUpdate (const PicturePtr&, const SurfacePtr&);
    bool pictureReferenceListSet (const PicturePtr&);

    void referenceListFree();
    //template end

    void resetGopStart();
    void setBFrame(const PicturePtr&);
    void setPFrame(const PicturePtr&);
    void setIFrame(const PicturePtr&);
    void setIdrFrame(const PicturePtr&);

    void changeLastBFrameToPFrame();

    YamiStatus encodeAllFrames();

    void resetParams();
    void checkProfileLimitation();
    void checkSvcTempLimitaion();

    VideoParamsAVC m_videoParamAVC;

    uint8_t m_levelIdc;
    uint32_t m_numSlices;
    uint32_t m_numBFrames;
    uint32_t m_mbWidth;
    uint32_t m_mbHeight;
    bool m_isSvcT;
    uint32_t m_temporalLayerNum;

    /* re-ordering */
    std::list<PicturePtr> m_reorderFrameList;
    VaapiEncReorderState m_reorderState;
    AVCStreamFormat m_streamFormat;
    uint32_t m_frameIndex;
    uint32_t m_curFrameNum;
    uint32_t m_keyPeriod;
    uint32_t m_ppsQp; /*pic_init_qp_minus26 + 26*/

    /* reference list */
    std::deque<ReferencePtr> m_refList;
    std::deque<ReferencePtr> m_refList0;
    std::deque<ReferencePtr> m_refList1;

    uint32_t m_maxRefFrames;
    /* max reflist count */
    uint32_t m_maxRefList0Count;
    uint32_t m_maxRefList1Count;

    /* frame, poc */
    uint32_t m_maxFrameNum;
    uint32_t m_log2MaxFrameNum;
    uint32_t m_maxPicOrderCnt;
    uint32_t m_log2MaxPicOrderCnt;
    uint16_t m_idrNum; //used to set idr_pic_id, max value is 65535 as spec

    VAEncSequenceParameterBufferH264* m_seqParam;
    VAEncPictureParameterBufferH264* m_picParam;

    StreamHeaderPtr m_headers;
    Lock m_paramLock; // locker for parameters update, for example: m_sps/m_pps/m_maxCodedbufSize (width/height etc)

    /**
     * VaapiEncoderFactory registration result. This encoder is registered in
     * vaapiencoder_host.cpp
     */
    static const bool s_registered;
};
}
#endif /* vaapiencoder_h264_h */
