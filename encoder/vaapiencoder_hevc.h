
/*
 *  vaapiencoder_hevc.h - hevc encoder for va
 *
 *  Copyright (C) 2014-2015 Intel Corporation
 *    Author: Wind Yuan <feng.yuan@intel.com>
 *    Author: Xu Guangxin <guangxin.xu@intel.com>
 *    Author: Li Zhong <zhong.li@intel.com>
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
    virtual Encode_Status start();
    virtual void flush();
    virtual Encode_Status stop();

    virtual Encode_Status getParameters(VideoParamConfigType type, Yami_PTR);
    virtual Encode_Status setParameters(VideoParamConfigType type, Yami_PTR);
    virtual Encode_Status getMaxOutSize(uint32_t *maxSize);
#ifdef __BUILD_GET_MV__
    // get MV buffer size.
    virtual Encode_Status getMVBufferSize(uint32_t * Size);
#endif

protected:
    virtual Encode_Status doEncode(const SurfacePtr&, uint64_t timeStamp, bool forceKeyFrame);
    virtual Encode_Status getCodecConfig(VideoEncOutputBuffer *outBuffer);

private:
    //following code is a template for other encoder implementation
    Encode_Status encodePicture(const PicturePtr&);
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
    Encode_Status reorder(const SurfacePtr& surface, uint64_t timeStamp, bool forceKeyFrame);
    bool referenceListUpdate (const PicturePtr&, const SurfacePtr&);
    bool pictureReferenceListSet (const PicturePtr&);

    void referenceListFree();
    //template end

    uint32_t& keyFramePeriod() {
        return m_videoParamAVC.idrInterval;
    }
    void resetGopStart();
    void setBFrame(const PicturePtr&);
    void setPFrame(const PicturePtr&);
    void setIFrame(const PicturePtr&);
    void setIdrFrame(const PicturePtr&);
    void setIntraFrame(const PicturePtr&, bool idIdr);
    void resetParams();
    void setShortRfs();
    void ShortRfsUpdate(const PicturePtr&);

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

    AVCStreamFormat m_streamFormat;

    /* re-ordering */
    std::list<PicturePtr> m_reorderFrameList;
    VaapiEncReorderState m_reorderState;
    uint32_t m_frameIndex;
    
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
    uint32_t m_idrNum;

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

    static const bool s_registered; // VaapiEncoderFactory registration result
};
}
#endif /* vaapiencoder_hevc_h */
