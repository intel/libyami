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
#include <pthread.h>
#include <tr1/memory>
#include <va/va_enc_h264.h>

namespace YamiMediaCodec{
class VaapiEncPictureH264;
class VaapiEncoderH264Ref;

class VaapiEncoderH264 : public VaapiEncoderBase {
public:
    //shortcuts, It's intended to elimilate codec diffrence
    //to make template for other codec implelmentation.
    typedef std::tr1::shared_ptr<VaapiEncPictureH264> PicturePtr;
    typedef std::tr1::shared_ptr<VaapiEncoderH264Ref> ReferencePtr;

    VaapiEncoderH264();
    ~VaapiEncoderH264();
    virtual Encode_Status start();
    virtual void flush();
    virtual Encode_Status stop();
    virtual Encode_Status getOutput(VideoEncOutputBuffer * outBuffer, bool withWait = false) const;

    virtual Encode_Status getParameters(VideoParamConfigSet *);
    virtual Encode_Status setParameters(VideoParamConfigSet *);
    virtual Encode_Status getMaxOutSize(uint32_t *maxSize);

protected:
    virtual Encode_Status reorder(const SurfacePtr&, uint64_t timeStamp, bool forceKeyFrame = false);
    virtual bool isBusy() { return m_outputQueue.size() >= m_maxOutputBuffer; } ;

private:
    //following code is a template for other encoder implementation
    Encode_Status submitEncode();
    Encode_Status encodePicture(const PicturePtr&,const CodedBufferPtr&);
    bool fill(VAEncSequenceParameterBufferH264*) const;
    bool fill(VAEncPictureParameterBufferH264*, const PicturePtr&, const CodedBufferPtr&, const SurfacePtr&) const ;
    bool addPackedSequenceHeader(const PicturePtr&, const VAEncSequenceParameterBufferH264* const);
    bool addPackedPictureHeader(const PicturePtr&, const VAEncPictureParameterBufferH264* const );
    bool addSliceHeaders (const PicturePtr&,
                          const std::vector<ReferencePtr>& refList0,
                          const std::vector<ReferencePtr>& refList1) const;
    bool ensureSequence(const PicturePtr&);
    bool ensurePicture (const PicturePtr&,const CodedBufferPtr&, const SurfacePtr&);
    bool ensureSlices(const PicturePtr&);
    Encode_Status getCodecCofnig(VideoEncOutputBuffer *outBuffer);

    //reference list related
    bool referenceListUpdate (const PicturePtr&, const SurfacePtr&);
    bool referenceListInit (
        const PicturePtr& ,
        std::vector<ReferencePtr>& reflist0,
        std::vector<ReferencePtr>& reflist1) const;

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
    uint32_t m_frameIndex;
    uint32_t m_curFrameNum;
    uint32_t m_curPresentIndex;
    /* reference list */
    std::list<ReferencePtr> m_refList;
    uint32_t m_maxRefFrames;
    /* max reflist count */
    uint32_t m_maxRefList0Count;
    uint32_t m_maxRefList1Count;

    /* output queue */
    mutable std::queue<std::pair<PicturePtr, CodedBufferPtr> >  m_outputQueue;
    mutable pthread_mutex_t m_outputQueueMutex;

    /* frame, poc */
    uint32_t m_maxFrameNum;
    uint32_t m_log2MaxFrameNum;
    uint32_t m_maxPicOrderCnt;
    uint32_t m_log2MaxPicOrderCnt;
    uint32_t m_idrNum;
    uint32_t m_maxCodedbufSize;

    std::vector<uint8_t> m_sps;
    std::vector<uint8_t> m_pps;
    Lock m_paramLock; // locker for parameters update, for example: sps/pps/m_maxCodedbufSize (width/height etc)
};
}
#endif /* vaapiencoder_h264_h */
