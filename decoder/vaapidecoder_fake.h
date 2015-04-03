/*
 *  vaapidecoder_fake.h
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

#ifndef vaapidecoder_fake_h
#define vaapidecoder_fake_h

#include "vaapidecoder_base.h"
#include "vaapidecpicture.h"


namespace YamiMediaCodec{
enum {
    FAKE_EXTRA_SURFACE_NUMBER = 8,
};

class VaapiDecoderFake:public VaapiDecoderBase {
  public:
    typedef SharedPtr<VaapiDecPicture> PicturePtr;
    VaapiDecoderFake(int32_t width, int32_t height);
    virtual ~ VaapiDecoderFake();
    virtual Decode_Status start(VideoConfigBuffer * );
    virtual Decode_Status decode(VideoDecodeBuffer *);

  private:
    int32_t m_width;
    int32_t m_height;
    bool    m_first;
};

};

#endif

