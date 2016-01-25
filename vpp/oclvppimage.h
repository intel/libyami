/*
 *  oclvppimage.h - image wrappers for opencl vpp modules
 *
 *  Copyright (C) 2016 Intel Corporation
 *    Author: Jia Meng<jia.meng@intel.com>
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
}
#endif /* oclvppimage_h */
