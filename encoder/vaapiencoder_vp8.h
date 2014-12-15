/*
 *  vaapiencoder_vp8.h - vp8 encoder for va
 *
 *  Copyright (C) 2014 Intel Corporation
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

#ifndef vaapiencoder_vp8_h
#define vaapiencoder_vp8_h

#include "vaapiencoder_base.h"
#include "vaapi/vaapiptrs.h"
#include <tr1/memory>
#include <va/va_enc_vp8.h>
#include <deque>


namespace YamiMediaCodec{

class VaapiEncPictureVP8;
class VaapiEncoderVP8 : public VaapiEncoderBase {
public:
    //shortcuts, It's intended to elimilate codec diffrence
    //to make template for other codec implelmentation.
    typedef std::tr1::shared_ptr<VaapiEncPictureVP8> PicturePtr;
    typedef SurfacePtr ReferencePtr;

    VaapiEncoderVP8();
    ~VaapiEncoderVP8();
    virtual Encode_Status start();
    virtual void flush();
    virtual Encode_Status stop();

    virtual Encode_Status getParameters(VideoParamConfigType type, Yami_PTR);
    virtual Encode_Status setParameters(VideoParamConfigType type, Yami_PTR);
    virtual Encode_Status getMaxOutSize(uint32_t *maxSize);

protected:
    virtual Encode_Status doEncode(const SurfacePtr&, uint64_t timeStamp, bool forceKeyFrame = false);

private:
    Encode_Status encodePicture(const PicturePtr&);
    bool fill(VAEncSequenceParameterBufferVP8*) const;
    bool fill(VAEncPictureParameterBufferVP8*, const PicturePtr&, const SurfacePtr&) const ;
    bool fill(VAQMatrixBufferVP8* qMatrix) const;
    bool ensureSequence(const PicturePtr&);
    bool ensurePicture (const PicturePtr&, const SurfacePtr&);
    bool ensureQMatrix (const PicturePtr&);
    bool referenceListUpdate (const PicturePtr&, const SurfacePtr&);

    void resetParams();

    int keyFramePeriod() { return m_videoParamCommon.intraPeriod ? m_videoParamCommon.intraPeriod : 1; }

    int m_frameCount;

    int m_maxCodedbufSize;

    typedef std::deque<SurfacePtr> ReferenceQueue;
    std::deque<SurfacePtr> m_reference;
};
}
#endif /* vaapiencoder_vp8_h */
