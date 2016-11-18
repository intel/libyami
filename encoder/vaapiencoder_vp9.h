/*
 * Copyright (C) 2016 Intel Corporation. All rights reserved.
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

#ifndef vaapiencoder_vp9_h
#define vaapiencoder_vp9_h

#include "vaapiencoder_base.h"
#include "vaapi/vaapiptrs.h"
#include <va/va_enc_vp9.h>
#include <deque>

#include "VideoEncoderDefs.h"

namespace YamiMediaCodec {

class VaapiEncPictureVP9;
class VaapiEncoderVP9 : public VaapiEncoderBase {
public:
    typedef SharedPtr<VaapiEncPictureVP9> PicturePtr;
    typedef SurfacePtr ReferencePtr;

    VaapiEncoderVP9();
    ~VaapiEncoderVP9();
    virtual YamiStatus start();
    virtual void flush();
    virtual YamiStatus stop();

    virtual YamiStatus getParameters(VideoParamConfigType type, Yami_PTR);
    virtual YamiStatus setParameters(VideoParamConfigType type, Yami_PTR);
    virtual YamiStatus getMaxOutSize(uint32_t* maxSize);

protected:
    virtual YamiStatus doEncode(const SurfacePtr&, uint64_t timeStamp,
                                bool forceKeyFrame = false);

private:
    friend class FactoryTest<IVideoEncoder, VaapiEncoderVP9>;
    friend class VaapiEncoderVP9Test;

    YamiStatus encodePicture(const PicturePtr&);
    bool fill(VAEncSequenceParameterBufferVP9*) const;
    bool fill(VAEncPictureParameterBufferVP9*, const PicturePtr&,
              const SurfacePtr&);
    bool fill(VAEncMiscParameterTypeVP9PerSegmantParam* segParam) const;
    bool ensureSequence(const PicturePtr&);
    bool ensurePicture(const PicturePtr&, const SurfacePtr&);
    bool ensureQMatrix(const PicturePtr&);
    bool referenceListUpdate(const PicturePtr&, const SurfacePtr&);

    YamiStatus resetParams();

    int keyFramePeriod()
    {
        return m_videoParamCommon.intraPeriod ? m_videoParamCommon.intraPeriod
                                              : 1;
    }

    inline uint32_t getReferenceMode() { return m_videoParamsVP9.referenceMode; }

    VideoParamsVP9 m_videoParamsVP9;
    int m_frameCount;
    int m_currentReferenceIndex;

    int m_maxCodedbufSize;

    typedef std::deque<SurfacePtr> ReferenceQueue;
    std::deque<SurfacePtr> m_reference;

    /**
     * VaapiEncoderFactory registration result. This encoder is registered in
     * vaapiencoder_host.cpp
     */
    static const bool s_registered;
};
} // namespace YamiMediaCodec
#endif // vaapiencoder_vp9_h
