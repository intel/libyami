/*
 *  vaapidecoder_vp9.h
 *
 *  Copyright (C) 2014 Intel Corporation
 *    Author: XuGuagnxin<Guangxin.Xu@intel.com>
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

#ifndef vaapidecoder_vp9_h
#define vaapidecoder_vp9_h

#include "codecparsers/vp9parser.h"
#include "vaapidecoder_base.h"
#include "vaapidecpicture.h"
#include "va/va_dec_vp9.h"
#include <vector>

namespace YamiMediaCodec{
enum {
    VP9_EXTRA_SURFACE_NUMBER = 5,
};

class VaapiDecoderVP9:public VaapiDecoderBase {
  public:
    typedef std::tr1::shared_ptr<VaapiDecPicture> PicturePtr;
    VaapiDecoderVP9();
    virtual ~ VaapiDecoderVP9();
    virtual Decode_Status start(VideoConfigBuffer * );
    virtual Decode_Status reset(VideoConfigBuffer * );
    virtual void stop(void);
    virtual void flush(void);
    virtual Decode_Status decode(VideoDecodeBuffer *);

  private:
    Decode_Status ensureContext(const Vp9FrameHdr* );
    Decode_Status decode(const uint8_t* data, uint32_t size, uint64_t timeStamp);
    Decode_Status decode(const Vp9FrameHdr* hdr, const uint8_t* data, uint32_t size, uint64_t timeStamp);
    bool ensureSlice(const PicturePtr& ,const Vp9FrameHdr* , const void* data, int size);
    bool ensurePicture(const PicturePtr& , const Vp9FrameHdr* );
    //reference related
    bool fillReference(VADecPictureParameterBufferVP9* , const Vp9FrameHdr*);
    void updateReference(const PicturePtr&, const Vp9FrameHdr*);
    uint32_t m_hasContext:1;
    typedef std::tr1::shared_ptr<Vp9Parser> ParserPtr;
    ParserPtr m_parser;
    std::vector<SurfacePtr> m_reference;
};

};

#endif
