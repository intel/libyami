
/*
 * Copyright (C) 2014-2016 Intel Corporation. All rights reserved.
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

#ifndef vaapiencoder_hevc_h
#define vaapiencoder_hevc_h

#include "vaapiencoder_base.h"
#include "vaapi/vaapiptrs.h"
#include "common/lock.h"
#include <list>
#include <queue>
#include <deque>
#include <pthread.h>
#include <va/va_enc_hevc.h>

namespace YamiMediaCodec{
class VaapiEncPictureHEVC;
class VaapiEncoderHEVCRef;
class VaapiEncStreamHeaderHEVC;

typedef struct shortRFS
{
    unsigned char    num_negative_pics;
    unsigned char    num_positive_pics;
    unsigned char    delta_poc_s0_minus1[8];
    unsigned char    used_by_curr_pic_s0_flag[8];
    unsigned char    delta_poc_s1_minus1[8];
    unsigned char    used_by_curr_pic_s1_flag[8];
    unsigned char    num_short_term_ref_pic_sets;
    unsigned char    short_term_ref_pic_set_idx;
    unsigned int     inter_ref_pic_set_prediction_flag;
}ShortRFS;

class VaapiEncoderHEVC : public VaapiEncoderBase {
    friend class VaapiEncStreamHeaderHEVC;
public:
    //shortcuts, It's intended to elimilate codec diffrence
    //to make template for other codec implelmentation.
    typedef SharedPtr<VaapiEncPictureHEVC> PicturePtr;
    typedef SharedPtr<VaapiEncoderHEVCRef> ReferencePtr;
    typedef SharedPtr <VaapiEncStreamHeaderHEVC> StreamHeaderPtr;

    VaapiEncoderHEVC();
    ~VaapiEncoderHEVC();
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

private:
    friend class FactoryTest<IVideoEncoder, VaapiEncoderHEVC>;
    friend class VaapiEncoderHEVCTest;

    //following code is a template for other encoder implementation
    YamiStatus encodePicture(const PicturePtr&);
    bool fill(VAEncSequenceParameterBufferHEVC*) const;
    bool fill(VAEncPictureParameterBufferHEVC*, const PicturePtr&, const SurfacePtr&) const ;
    bool fillReferenceList(VAEncSliceParameterBufferHEVC* slice) const;
    bool ensureSequenceHeader(const PicturePtr&, const VAEncSequenceParameterBufferHEVC* const);
    bool ensurePictureHeader(const PicturePtr&, const VAEncPictureParameterBufferHEVC* const );
    bool addSliceHeaders (const PicturePtr&) const;
    bool addPackedSliceHeader (const PicturePtr&,
                          const VAEncSliceParameterBufferHEVC* const sliceParam,
                          uint32_t sliceIndex) const;
    bool ensureSequence(const PicturePtr&);
    bool ensurePicture (const PicturePtr&, const SurfacePtr&);
    bool ensureSlices(const PicturePtr&);
    bool ensureCodedBufferSize();

    //reference list related
    YamiStatus reorder(const SurfacePtr& surface, uint64_t timeStamp, bool forceKeyFrame);
    bool referenceListUpdate (const PicturePtr&, const SurfacePtr&);
    bool pictureReferenceListSet (const PicturePtr&);

    void referenceListFree();
    //template end

    void resetGopStart();
    void setBFrame(const PicturePtr&);
    void setPFrame(const PicturePtr&);
    void setIFrame(const PicturePtr&);
    void setIdrFrame(const PicturePtr&);
    void setIntraFrame(const PicturePtr&, bool idIdr);
    void resetParams();
    void setShortRfs();
    void shortRfsUpdate(const PicturePtr&);

    void changeLastBFrameToPFrame();
    YamiStatus encodeAllFrames();

    VideoParamsAVC m_videoParamAVC;

    uint8_t m_profileIdc;
    uint8_t m_levelIdc;
    uint32_t m_numSlices;
    uint32_t m_numBFrames;
    uint32_t m_ctbSize;
    uint32_t m_cuSize;
    uint32_t m_minTbSize;
    uint32_t m_maxTbSize;

    uint32_t m_AlignedWidth;
    uint32_t m_AlignedHeight;
    uint32_t m_cuWidth;
    uint32_t m_cuHeight;

    /* re-ordering */
    std::list<PicturePtr> m_reorderFrameList;
    VaapiEncReorderState m_reorderState;
    uint32_t m_frameIndex;
    uint32_t m_keyPeriod;
    
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

    bool m_confWinFlag;
    uint32_t m_confWinLeftOffset;
    uint32_t m_confWinRightOffset;
    uint32_t m_confWinTopOffset;
    uint32_t m_confWinBottomOffset;

    VAEncSequenceParameterBufferHEVC* m_seqParam;
    VAEncPictureParameterBufferHEVC* m_picParam;

    ShortRFS m_shortRFS;
    StreamHeaderPtr m_headers;
    Lock m_paramLock; // locker for parameters update, for example: m_sps/m_pps/m_maxCodedbufSize (width/height etc)

    /**
     * VaapiEncoderFactory registration result. This encoder is registered in
     * vaapiencoder_host.cpp
     */
    static const bool s_registered;
};
}
#endif /* vaapiencoder_hevc_h */
