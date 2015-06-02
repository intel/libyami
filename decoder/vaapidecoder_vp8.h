/*
 *  vaapidecoder_vp8.h
 *
 *  Copyright (C) 2013-2014 Intel Corporation
 *    Author: Zhao, Halley<halley.zhao@intel.com>
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

#ifndef vaapidecoder_vp8_h
#define vaapidecoder_vp8_h

#include "codecparsers/vp8parser.h"
#include "vaapidecoder_base.h"
#include "vaapidecpicture.h"
#include "va/va_dec_vp8.h"

#if __PLATFORM_BYT__
#define __PSB_CACHE_DRAIN_FOR_FIRST_FRAME__ 1
#define __PSB_VP8_INTERFACE_WORK_AROUND__   0
#else
#define __PSB_CACHE_DRAIN_FOR_FIRST_FRAME__ 0
#define __PSB_VP8_INTERFACE_WORK_AROUND__   0
#endif

namespace YamiMediaCodec{
enum {
    VP8_EXTRA_SURFACE_NUMBER = 5,
    VP8_MAX_PICTURE_COUNT = 5,  // gold_ref, alt_ref, last_ref, previous (m_currentPicture, optional), and the newly allocated one
};

class VaapiDecoderVP8:public VaapiDecoderBase {
  public:
    typedef SharedPtr<VaapiDecPicture> PicturePtr;
    VaapiDecoderVP8();
    virtual ~ VaapiDecoderVP8();
    virtual Decode_Status start(VideoConfigBuffer * buffer);
    virtual Decode_Status reset(VideoConfigBuffer * buffer);
    virtual void stop(void);
    virtual void flush(void);
    virtual Decode_Status decode(VideoDecodeBuffer * buffer);

  private:
    bool allocNewPicture();
    bool fillPictureParam(const PicturePtr& picture);
    /* fill Quant matrix parameters */
    bool ensureQuantMatrix(const PicturePtr& pic);
    bool ensureProbabilityTable(const PicturePtr& pic);
    bool fillSliceParam(VASliceParameterBufferVP8* sliceParam);
    /* check the context reset senerios */
    Decode_Status ensureContext();
    /* decoding functions */
    Decode_Status decodePicture();
    void updateReferencePictures();
  private:
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
    Vp8FrameHdr m_frameHdr;
    Vp8Parser m_parser;
    uint8_t m_yModeProbs[4];
    uint8_t m_uvModeProbs[3];
    uint32_t m_sizeChanged:1;

#if __PSB_CACHE_DRAIN_FOR_FIRST_FRAME__
    bool m_isFirstFrame;
#endif

    static const bool s_registered; // VaapiDecoderFactory registration result
};
}

#endif
