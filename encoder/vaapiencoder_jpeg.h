/*
 * Copyright 2016 Intel Corporation
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

#ifndef vaapiencoder_jpeg_h
#define vaapiencoder_jpeg_h

#include "vaapiencoder_base.h"
#include "vaapi/vaapiptrs.h"
#include <va/va_enc_jpeg.h>

namespace YamiMediaCodec {
class VaapiEncPictureJPEG;

class VaapiEncoderJpeg : public VaapiEncoderBase {
public:
    typedef SharedPtr<VaapiEncPictureJPEG> PicturePtr;

    VaapiEncoderJpeg();
    virtual ~VaapiEncoderJpeg() { }
    virtual YamiStatus start();
    virtual void flush();
    virtual YamiStatus stop();

    virtual YamiStatus getParameters(VideoParamConfigType type, Yami_PTR videoEncParams);
    virtual YamiStatus setParameters(VideoParamConfigType type, Yami_PTR videoEncParams);
    virtual YamiStatus getMaxOutSize(uint32_t* maxSize);

protected:
    virtual YamiStatus doEncode(const SurfacePtr&, uint64_t timeStamp, bool forceKeyFrame);
    virtual bool isBusy() { return false;};

private:
    friend class FactoryTest<IVideoEncoder, VaapiEncoderJpeg>;
    friend class VaapiEncoderJpegTest;

    YamiStatus encodePicture(const PicturePtr&);
    bool addSliceHeaders (const PicturePtr&) const;
    bool fill(VAEncPictureParameterBufferJPEG * picParam, const PicturePtr &, const SurfacePtr &) const;
    bool fill(VAQMatrixBufferJPEG * qMatrix) const;
    bool fill(VAEncSliceParameterBufferJPEG *sliceParam) const;
    bool fill(VAHuffmanTableBufferJPEGBaseline *huffTableParam) const;

    bool ensurePicture (const PicturePtr&, const SurfacePtr&);
    bool ensureQMatrix (const PicturePtr&);
    bool ensureSlice (const PicturePtr&);
    bool ensureHuffTable (const PicturePtr&);

    void resetParams();

    unsigned char quality;

    /**
     * VaapiEncoderFactory registration result. This encoder is registered in
     * vaapiencoder_host.cpp
     */
    static const bool s_registered;
};
}
#endif
