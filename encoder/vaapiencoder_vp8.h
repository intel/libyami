/*
 * Copyright (C) 2014 Intel Corporation. All rights reserved.
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
class VaapiEncoderVP8 : public VaapiEncoderBase {
public:
    //shortcuts, It's intended to elimilate codec diffrence
    //to make template for other codec implelmentation.
    typedef SharedPtr<VaapiEncPictureVP8> PicturePtr;
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
    friend class FactoryTest<IVideoEncoder, VaapiEncoderVP8>;
    friend class VaapiEncoderVP8Test;

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

    int m_qIndex;

    typedef std::deque<SurfacePtr> ReferenceQueue;
    std::deque<SurfacePtr> m_reference;

    static const bool s_registered; // VaapiEncoderFactory registration result
};
}
#endif /* vaapiencoder_vp8_h */
