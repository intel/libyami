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
    virtual YamiStatus start(VideoConfigBuffer*);
    virtual YamiStatus decode(VideoDecodeBuffer*);

  private:
    int32_t m_width;
    int32_t m_height;
    bool    m_first;
};

};

#endif

