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

#ifndef vaapiencoder_vp8_h
#define vaapiencoder_vp8_h

#include "vaapiencoder_base.h"
#include "vaapi/vaapiptrs.h"
#include <va/va_enc_vp8.h>
#include <deque>


namespace YamiMediaCodec{

class VaapiEncPictureVP8;
class Vp8Encoder;
struct RefFlags;
class VaapiEncoderVP8 : public VaapiEncoderBase {
public:
    //shortcuts, It's intended to elimilate codec diffrence
    //to make template for other codec implelmentation.
    typedef SharedPtr<VaapiEncPictureVP8> PicturePtr;
    typedef SharedPtr<Vp8Encoder> Vp8EncoderPtr;
    typedef SurfacePtr ReferencePtr;

    VaapiEncoderVP8();
    ~VaapiEncoderVP8();
    virtual YamiStatus start();
    virtual void flush();
    virtual YamiStatus stop();

    virtual YamiStatus getParameters(VideoParamConfigType type, Yami_PTR);
    virtual YamiStatus setParameters(VideoParamConfigType type, Yami_PTR);
    virtual YamiStatus getMaxOutSize(uint32_t* maxSize);

protected:
    virtual YamiStatus doEncode(const SurfacePtr&, uint64_t timeStamp, bool forceKeyFrame = false);

private:
    friend class FactoryTest<IVideoEncoder, VaapiEncoderVP8>;
    friend class VaapiEncoderVP8Test;

    YamiStatus encodePicture(const PicturePtr&);
    bool fill(VAEncSequenceParameterBufferVP8*) const;
    void fill(VAEncPictureParameterBufferVP8*, const RefFlags&) const;
    bool fill(VAEncPictureParameterBufferVP8*, const PicturePtr&, const SurfacePtr&, RefFlags&) const;
    bool fill(VAQMatrixBufferVP8* qMatrix) const;
    bool ensureSequence(const PicturePtr&);
    bool ensurePicture (const PicturePtr&, const SurfacePtr&, RefFlags&);
    bool ensureQMatrix (const PicturePtr&);
    const SurfacePtr& referenceUpdate(const SurfacePtr& to, const SurfacePtr& from,
        const SurfacePtr& recon, bool refresh, uint32_t copy) const;
    bool referenceListUpdate (const PicturePtr&, const SurfacePtr&, const RefFlags&);

    void resetParams();

    int keyFramePeriod() { return m_videoParamCommon.intraPeriod ? m_videoParamCommon.intraPeriod : 1; }

    int m_frameCount;

    int m_maxCodedbufSize;

    int m_qIndex;

    SurfacePtr m_last;
    SurfacePtr m_golden;
    SurfacePtr m_alt;
    Vp8EncoderPtr m_encoder;

    /**
     * VaapiEncoderFactory registration result. This encoder is registered in
     * vaapiencoder_host.cpp
     */
    static const bool s_registered;
};
}
#endif /* vaapiencoder_vp8_h */
