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

#ifndef oclvppimage_h
#define oclvppimage_h

#include "VideoCommonDefs.h"
#include <CL/opencl.h>
#include <va/va.h>

namespace YamiMediaCodec {
/**
 * \class OclVppImage
 * \brief Image wrappers used by opencl vpp modules
 *
 */
template <typename MemType>
class OclVppImage {
public:
    const MemType& plane(int n) { return m_mem[n]; }
    uint32_t pitch(int n) { return m_image.pitches[n]; }
    uint32_t numPlanes() { return m_image.num_planes; }
    uint32_t getWidth() { return m_image.width; }
    uint32_t getHeight() { return m_image.height; }
    uint32_t getFourcc() { return m_frame->fourcc; }

protected:
    OclVppImage(VADisplay d, const SharedPtr<VideoFrame>& f)
        : m_display(d)
        , m_frame(f)
    {
        memset(&m_image, 0, sizeof(m_image));
        m_image.image_id = VA_INVALID_ID;
        m_image.buf = VA_INVALID_ID;
    }
    virtual bool init() = 0;

    virtual ~OclVppImage() {}
    VADisplay m_display;
    SharedPtr<VideoFrame> m_frame;
    VAImage m_image;
    MemType m_mem[3];
};

class OclVppRawImage : public OclVppImage<uint8_t*> {
public:
    static SharedPtr<OclVppRawImage> create(VADisplay d, const SharedPtr<VideoFrame>& f);
    ~OclVppRawImage();
    uint8_t pixel(uint32_t x, uint32_t y, int n) { return m_mem[n][y * pitch(n) + x]; }

private:
    OclVppRawImage(VADisplay d, const SharedPtr<VideoFrame>& f)
        : OclVppImage(d, f)
    {
    }
    virtual bool init();
};

class OclVppCLImage : public OclVppImage<cl_mem> {
public:
    static SharedPtr<OclVppCLImage> create(VADisplay d,
        const SharedPtr<VideoFrame>& f,
        const SharedPtr<OclContext> ctx,
        const cl_image_format& fmt);
    ~OclVppCLImage();

private:
    OclVppCLImage(VADisplay d,
        const SharedPtr<VideoFrame>& f,
        const SharedPtr<OclContext> ctx,
        const cl_image_format& fmt)
        : OclVppImage(d, f)
        , m_context(ctx)
        , m_fmt(fmt)
    {
    }
    virtual bool init();

    SharedPtr<OclContext> m_context;
    const cl_image_format& m_fmt;
};

class OclMemDeleter {
public:
    void operator()(cl_mem* mem)
    {
        checkOclStatus(clReleaseMemObject(*mem), "ReleaseMemObject");
        delete mem;
    }
};
}
#endif /* oclvppimage_h */
