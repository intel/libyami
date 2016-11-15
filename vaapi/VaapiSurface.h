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

#ifndef VaapiSurface_h
#define VaapiSurface_h

#include "common/NonCopyable.h"
#include "VideoCommonDefs.h"
#include <va/va.h>
#include <stdint.h>

namespace YamiMediaCodec {

class VaapiSurface {
    friend class VaapiDecSurfacePool;

public:
    VaapiSurface(intptr_t id, uint32_t width, uint32_t height, uint32_t fourcc);
    VaapiSurface(const SharedPtr<VideoFrame>&);

    VASurfaceID getID();

    bool setCrop(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    void getCrop(uint32_t& x, uint32_t& y, uint32_t& width, uint32_t& height);
    uint32_t getFourcc();

private:
    SharedPtr<VideoFrame> m_frame;
    uint32_t m_width;
    uint32_t m_height;
    DISALLOW_COPY_AND_ASSIGN(VaapiSurface);
};

}

#endif //VaapiSurface_h
