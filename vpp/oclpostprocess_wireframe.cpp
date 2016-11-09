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

#include "oclpostprocess_wireframe.h"
#include "common/common_def.h"
#include "common/log.h"
#include "ocl/oclcontext.h"
#include "vpp/oclvppimage.h"

namespace YamiMediaCodec {

YamiStatus
OclPostProcessWireframe::process(const SharedPtr<VideoFrame>& src,
    const SharedPtr<VideoFrame>& dst)
{
    YamiStatus status = ensureContext("wireframe");
    if (status != YAMI_SUCCESS)
        return status;

    if (src != dst) {
        ERROR("src and dst must be the same for wireframe");
        return YAMI_INVALID_PARAM;
    }

    if (dst->fourcc != YAMI_FOURCC_NV12) {
        ERROR("only support wireframe on NV12 video frame");
        return YAMI_INVALID_PARAM;
    }

    if (m_borderWidth == 0
        || dst->crop.width < 2 * m_borderWidth
        || dst->crop.height < 2 * m_borderWidth) {
        ERROR("wireframe invalid param");
        return YAMI_INVALID_PARAM;
    }

    cl_image_format format;
    format.image_channel_order = CL_RG;
    format.image_channel_data_type = CL_UNSIGNED_INT8;
    uint32_t pixelSize = getPixelSize(format);
    uint32_t x = ALIGN_POW2(dst->crop.x, pixelSize) / pixelSize;
    uint32_t y = ALIGN_POW2(dst->crop.y, pixelSize) / pixelSize;
    uint32_t w = ALIGN_POW2(dst->crop.width, pixelSize) / pixelSize;
    uint32_t h = ALIGN_POW2(dst->crop.height, pixelSize) / pixelSize;
    uint32_t b = ALIGN_POW2(m_borderWidth, pixelSize) / pixelSize;
    SharedPtr<OclVppCLImage> imagePtr = OclVppCLImage::create(m_display, dst, m_context, format);
    if (!imagePtr->numPlanes()) {
        ERROR("failed to create cl image from dst frame");
        return YAMI_FAIL;
    }

    size_t globalWorkSize[2], localWorkSize[2];
    localWorkSize[0] = 8;
    localWorkSize[1] = 8;
    globalWorkSize[0] = (w / localWorkSize[0] + 1) * localWorkSize[0];
    globalWorkSize[1] = (h / localWorkSize[1] + 1) * localWorkSize[1];

    if ((CL_SUCCESS != clSetKernelArg(m_kernelWireframe, 0, sizeof(cl_mem), &imagePtr->plane(0)))
         || (CL_SUCCESS != clSetKernelArg(m_kernelWireframe, 1, sizeof(cl_mem), &imagePtr->plane(1)))
         || (CL_SUCCESS != clSetKernelArg(m_kernelWireframe, 2, sizeof(uint32_t), &x))
         || (CL_SUCCESS != clSetKernelArg(m_kernelWireframe, 3, sizeof(uint32_t), &y))
         || (CL_SUCCESS != clSetKernelArg(m_kernelWireframe, 4, sizeof(uint32_t), &w))
         || (CL_SUCCESS != clSetKernelArg(m_kernelWireframe, 5, sizeof(uint32_t), &h))
         || (CL_SUCCESS != clSetKernelArg(m_kernelWireframe, 6, sizeof(uint32_t), &b))
         || (CL_SUCCESS != clSetKernelArg(m_kernelWireframe, 7, sizeof(uint8_t), &m_colorY))
         || (CL_SUCCESS != clSetKernelArg(m_kernelWireframe, 8, sizeof(uint8_t), &m_colorU))
         || (CL_SUCCESS != clSetKernelArg(m_kernelWireframe, 9, sizeof(uint8_t), &m_colorV))) {
        ERROR("clSetKernelArg failed");
        return YAMI_FAIL;
    }

    if (!checkOclStatus(clEnqueueNDRangeKernel(m_context->m_queue, m_kernelWireframe, 2, NULL,
                            globalWorkSize, localWorkSize, 0, NULL, NULL),
            "EnqueueNDRangeKernel")) {
        return YAMI_FAIL;
    }

    return status;
}

YamiStatus OclPostProcessWireframe::setParameters(VppParamType type, void* vppParam)
{
    YamiStatus status = YAMI_INVALID_PARAM;

    switch (type) {
    case VppParamTypeWireframe: {
        VppParamWireframe* wireframe = (VppParamWireframe*)vppParam;
        if (wireframe->size == sizeof(VppParamWireframe)) {
            m_borderWidth = ALIGN2(wireframe->borderWidth);
            if (m_borderWidth == 0 || m_borderWidth != wireframe->borderWidth)
                ERROR("wireframe border width must be non-zero and 2-pixel aligned");
            m_colorY = wireframe->colorY;
            m_colorU = wireframe->colorU;
            m_colorV = wireframe->colorV;
            status = YAMI_SUCCESS;
        }
    } break;
    default:
        status = OclPostProcessBase::setParameters(type, vppParam);
        break;
    }
    return status;
}

bool OclPostProcessWireframe::prepareKernels()
{
    m_kernelWireframe = prepareKernel("wireframe");

    return m_kernelWireframe != NULL;
}

}
