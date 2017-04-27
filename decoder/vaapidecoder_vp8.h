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

#ifndef vaapidecoder_vp8_h
#define vaapidecoder_vp8_h

#include "codecparsers/vp8_parser.h"
#include "vaapidecoder_base.h"
#include "vaapidecpicture.h"
#include "va/va_dec_vp8.h"

#if __PLATFORM_BYT__
#define __PSB_CACHE_DRAIN_FOR_FIRST_FRAME__ 0
#define __PSB_VP8_INTERFACE_WORK_AROUND__   1
#else
#define __PSB_CACHE_DRAIN_FOR_FIRST_FRAME__ 0
#define __PSB_VP8_INTERFACE_WORK_AROUND__   0
#endif

namespace YamiMediaCodec{

// function below taken from:
// https://src.chromium.org/svn/trunk/src/third_party/cld/base/macros.h
// The arraysize(arr) macro returns the # of elements in an array arr.
// The expression is a compile-time constant, and therefore can be
// used in defining new arrays, for example.  If you use arraysize on
// a pointer by mistake, you will get a compile-time error.
//
// One caveat is that arraysize() doesn't accept any array of an
// anonymous type or a type defined inside a function. This is
// due to a limitation in C++'s template system.  The limitation might
// eventually be removed, but it hasn't happened yet.

// This template function declaration is used in defining arraysize.
// Note that the function doesn't need an implementation, as we only
// use its type.
template <typename T, size_t N>
char (&ArraySizeHelper(T (&array)[N]))[N];

// That gcc wants both of these prototypes seems mysterious. VC, for
// its part, can't decide which to use (another mystery). Matching of
// template overloads: the final frontier.
template <typename T, size_t N>
char (&ArraySizeHelper(const T (&array)[N]))[N];

#define arraysize(array) (sizeof(ArraySizeHelper(array)))

enum {
    VP8_MAX_PICTURE_COUNT = 5,  // gold_ref, alt_ref, last_ref, previous (m_currentPicture, optional), and the newly allocated one
};

class VaapiDecoderVP8:public VaapiDecoderBase {
  public:
    typedef SharedPtr<VaapiDecPicture> PicturePtr;
    VaapiDecoderVP8();
    virtual ~ VaapiDecoderVP8();
    virtual YamiStatus start(VideoConfigBuffer* buffer);
    virtual YamiStatus reset(VideoConfigBuffer* buffer);
    virtual void stop(void);
    virtual void flush(void);
    virtual YamiStatus decode(VideoDecodeBuffer* buffer);
    bool targetTemporalFrame();

  private:
      YamiStatus allocNewPicture();
    bool fillPictureParam(const PicturePtr& picture);
    /* fill Quant matrix parameters */
    bool ensureQuantMatrix(const PicturePtr& pic);
    bool ensureProbabilityTable(const PicturePtr& pic);
    bool fillSliceParam(VASliceParameterBufferVP8* sliceParam);
    /* check the context reset senerios */
    YamiStatus ensureContext();
    /* decoding functions */
    YamiStatus decodePicture();
    void updateReferencePictures();
    void flush(bool discardOutput);

  private:
    friend class FactoryTest<IVideoDecoder, VaapiDecoderVP8>;
    friend class VaapiDecoderVP8Test;

    PicturePtr m_currentPicture;
    PicturePtr m_lastPicture;
    PicturePtr m_goldenRefPicture;
    PicturePtr m_altRefPicture;

    uint32_t m_hasContext:1;

    // resolution of current frame, VP8 may change frame resolution starting with a key frame
    uint32_t m_frameWidth;
    uint32_t m_frameHeight;
    const uint8_t *m_buffer;
    uint32_t m_frameSize;
    YamiParser::Vp8FrameHeader m_frameHdr;
    YamiParser::Vp8Parser m_parser;
    uint8_t m_yModeProbs[4];
    uint8_t m_uvModeProbs[3];
    uint32_t m_sizeChanged:1;
    //we can not decode P frame if we do not get key.
    bool m_gotKeyFrame;

#if __PSB_CACHE_DRAIN_FOR_FIRST_FRAME__
    bool m_isFirstFrame;
#endif

    /**
     * VaapiDecoderFactory registration result. This decoder is registered in
     * vaapidecoder_host.cpp
     */
    static const bool s_registered;
};
} // namespace YamiMediaCodec

#endif
