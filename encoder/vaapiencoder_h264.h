
/*
 *  vaapiencoder_h264.h - h264 encoder for va
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
    bool fill(VAEncSequenceParameterBufferH264*) const;
    bool fill(VAEncPictureParameterBufferH264*, const PicturePtr&, const SurfacePtr&) const ;
    bool ensureSequenceHeader(const PicturePtr&, const VAEncSequenceParameterBufferH264* const);
    bool ensurePictureHeader(const PicturePtr&, const VAEncPictureParameterBufferH264* const );
    bool addSliceHeaders (const PicturePtr&) const;
    bool ensureSequence(const PicturePtr&);
    bool ensurePicture (const PicturePtr&, const SurfacePtr&);
    bool ensureSlices(const PicturePtr&);
    bool ensureCodedBufferSize();

    //reference list related
    Encode_Status reorder(const SurfacePtr& surface, uint64_t timeStamp, bool forceKeyFrame);
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

    void resetParams();

    VideoParamsAVC m_videoParamAVC;

    uint8_t m_levelIdc;
    uint32_t m_numSlices;
    uint32_t m_numBFrames;
    uint32_t m_mbWidth;
    uint32_t m_mbHeight;
    bool  m_useCabac;
    bool  m_useDct8x8;

    /* re-ordering */
    std::list<PicturePtr> m_reorderFrameList;
    VaapiEncReorderState m_reorderState;
    AVCStreamFormat m_streamFormat;
    uint32_t m_frameIndex;
    uint32_t m_curFrameNum;
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
    uint16_t m_idrNum; //used to set idr_pic_id, max value is 65535 as spec

    StreamHeaderPtr m_headers;
    Lock m_paramLock; // locker for parameters update, for example: m_sps/m_pps/m_maxCodedbufSize (width/height etc)

    static const bool s_registered; // VaapiEncoderFactory registration result
};
}
#endif /* vaapiencoder_h264_h */
