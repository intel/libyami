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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common/log.h"
#include "ocl/oclcontext.h"
#include "oclvppimage.h"
#include "vaapi/VaapiUtils.h"
#include <va/va_drmcommon.h>

namespace YamiMediaCodec {

SharedPtr<OclVppRawImage> OclVppRawImage::create(
    VADisplay d,
    const SharedPtr<VideoFrame>& f)
{
    SharedPtr<OclVppRawImage> image(new OclVppRawImage(d, f));
    if (image->init())
        return image;
    ERROR("Failed to create OclVppRawImage");
    image.reset();
    return image;
}

bool OclVppRawImage::init()
{
    VASurfaceID surfaceId = (VASurfaceID)m_frame->surface;
    if (!checkVaapiStatus(vaDeriveImage(m_display, surfaceId, &m_image), "DeriveImage"))
        return false;

    uint8_t* buf = 0;
    if (!checkVaapiStatus(vaMapBuffer(m_display, m_image.buf, (void**)&buf), "vaMapBuffer"))
        return false;

    for (uint32_t n = 0; n < m_image.num_planes; n++) {
        m_mem[n] = buf + m_image.offsets[n];
    }
    return true;
}

OclVppRawImage::~OclVppRawImage()
{
    checkVaapiStatus(vaUnmapBuffer(m_display, m_image.buf), "ReleaseBufferHandle");
    checkVaapiStatus(vaDestroyImage(m_display, m_image.image_id), "DestroyImage");
}

SharedPtr<OclVppCLImage> OclVppCLImage::create(
    VADisplay d,
    const SharedPtr<VideoFrame>& f,
    const SharedPtr<OclContext> ctx,
    const cl_image_format& fmt)
{
    SharedPtr<OclVppCLImage> image(new OclVppCLImage(d, f, ctx, fmt));
    if (image->init())
        return image;
    ERROR("Failed to create OclVppCLImage");
    image.reset();
    return image;
}

bool OclVppCLImage::init()
{
    VASurfaceID surfaceId = (VASurfaceID)m_frame->surface;
    if (!checkVaapiStatus(vaDeriveImage(m_display, surfaceId, &m_image), "DeriveImage"))
        return false;

    VABufferInfo bufferInfo;
    bufferInfo.mem_type = VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME;
    if (!checkVaapiStatus(vaAcquireBufferHandle(m_display, m_image.buf, &bufferInfo),
            "AcquireBufferHandle"))
        return false;

    cl_import_image_info_intel importInfo;
    uint32_t height[3] = {0};
    switch (m_image.format.fourcc) {
    case VA_FOURCC_RGBA:
        height[0] = m_image.height;
        break;
    case VA_FOURCC_NV12:
        height[0] = m_image.height;
        height[1] = m_image.height / 2;
        break;
    default:
        ERROR("unsupported format");
        return false;
    }

    for (uint32_t n = 0; n < m_image.num_planes; n++) {
        importInfo.fd = bufferInfo.handle;
        importInfo.type = CL_MEM_OBJECT_IMAGE2D;
        importInfo.fmt.image_channel_order = m_fmt.image_channel_order;
        importInfo.fmt.image_channel_data_type = m_fmt.image_channel_data_type;
        importInfo.row_pitch = m_image.pitches[n];
        importInfo.offset = m_image.offsets[n];
        importInfo.width = m_image.width;
        importInfo.height = height[n];
        importInfo.size = importInfo.row_pitch * importInfo.height;
        if (YAMI_SUCCESS != m_context->createImageFromFdIntel(&importInfo, &m_mem[n])) {
            ERROR("createImageFromFdIntel failed");
            return false;
        }
    }

    return true;
}

OclVppCLImage::~OclVppCLImage()
{
    for (uint32_t n = 0; n < m_image.num_planes; n++)
        checkOclStatus(clReleaseMemObject(m_mem[n]), "ReleaseMemObject");

    checkVaapiStatus(vaReleaseBufferHandle(m_display, m_image.buf), "ReleaseBufferHandle");
    checkVaapiStatus(vaDestroyImage(m_display, m_image.image_id), "DestroyImage");
}
}
