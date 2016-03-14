/*
 *  oclpostprocess_blend.cpp - ocl based alpha blending
 *
 *  Copyright (C) 2015 Intel Corporation
 *    Author: XuGuangxin<Guangxin.Xu@intel.com>
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common/common_def.h"
#include "common/log.h"
#include "ocl/oclcontext.h"
#include "oclpostprocess_blender.h"
#include "oclvppimage.h"
#include "vaapipostprocess_factory.h"

namespace YamiMediaCodec{

YamiStatus
OclPostProcessBlender::blend(const SharedPtr<VideoFrame>& src,
                             const SharedPtr<VideoFrame>& dst)
{
    YamiStatus ret = YAMI_SUCCESS;
    SharedPtr<OclVppCLImage> srcImagePtr, dstImagePtr;
    cl_image_format srcFormat, dstFormat;

    srcFormat.image_channel_order = CL_RGBA;
    srcFormat.image_channel_data_type = CL_UNORM_INT8;
    srcImagePtr = OclVppCLImage::create(m_display, src, m_context, srcFormat);
    if (!srcImagePtr) {
        ERROR("failed to create cl image from src frame");
        return YAMI_FAIL;
    }

    dstFormat.image_channel_order = CL_RG;
    dstFormat.image_channel_data_type = CL_UNORM_INT8;
    dstImagePtr = OclVppCLImage::create(m_display, dst, m_context, dstFormat);
    if (!dstImagePtr) {
        ERROR("failed to create cl image from dst frame");
        return YAMI_FAIL;
    }

    cl_mem bgImageMem[3];
    for (uint32_t n = 0; n < dstImagePtr->numPlanes(); n++) {
        bgImageMem[n] = dstImagePtr->plane(n);
    }

    uint32_t pixelSize = getPixelSize(dstFormat);
    VideoRect crop;
    crop.x = dst->crop.x / pixelSize;
    crop.y = dst->crop.y & ~1;
    crop.width = dst->crop.width / pixelSize;
    crop.height = dst->crop.height;
    if ((CL_SUCCESS != clSetKernelArg(m_kernelBlend, 0, sizeof(cl_mem), &dstImagePtr->plane(0)))
        || (CL_SUCCESS != clSetKernelArg(m_kernelBlend, 1, sizeof(cl_mem), &dstImagePtr->plane(1)))
        || (CL_SUCCESS != clSetKernelArg(m_kernelBlend, 2, sizeof(cl_mem), &bgImageMem[0]))
        || (CL_SUCCESS != clSetKernelArg(m_kernelBlend, 3, sizeof(cl_mem), &bgImageMem[1]))
        || (CL_SUCCESS != clSetKernelArg(m_kernelBlend, 4, sizeof(cl_mem), &srcImagePtr->plane(0)))
        || (CL_SUCCESS != clSetKernelArg(m_kernelBlend, 5, sizeof(uint32_t), &crop.x))
        || (CL_SUCCESS != clSetKernelArg(m_kernelBlend, 6, sizeof(uint32_t), &crop.y))
        || (CL_SUCCESS != clSetKernelArg(m_kernelBlend, 7, sizeof(uint32_t), &crop.width))
        || (CL_SUCCESS != clSetKernelArg(m_kernelBlend, 8, sizeof(uint32_t), &crop.height))) {
        ERROR("clSetKernelArg failed");
        return YAMI_FAIL;
    }

    size_t globalWorkSize[2], localWorkSize[2];
    // each work group has 8x8 work items; each work item handles 2x2 pixels
    localWorkSize[0] = 8;
    localWorkSize[1] = 8;
    globalWorkSize[0] = ALIGN16(dst.get()->crop.width) / pixelSize;
    globalWorkSize[1] = ALIGN16(dst.get()->crop.height) / 2;
    if (!checkOclStatus(clEnqueueNDRangeKernel(m_context->m_queue, m_kernelBlend, 2, NULL,
                            globalWorkSize, localWorkSize, 0, NULL, NULL),
            "EnqueueNDRangeKernel")) {
        return YAMI_FAIL;
    }

    return ret;
}

YamiStatus
OclPostProcessBlender::process(const SharedPtr<VideoFrame>& src,
                               const SharedPtr<VideoFrame>& dest)
{
    YamiStatus status = ensureContext("blend");
    if (status != YAMI_SUCCESS)
        return status;

    if (src->fourcc != YAMI_FOURCC_RGBA || dest->fourcc != YAMI_FOURCC_NV12) {
        ERROR("only support RGBA blending on NV12 video frame");
        return YAMI_INVALID_PARAM;
    }

    return blend(src, dest);
}

bool OclPostProcessBlender::prepareKernels()
{
    m_kernelBlend = prepareKernel("blend");

    return m_kernelBlend != NULL;
}

const bool OclPostProcessBlender::s_registered =
    VaapiPostProcessFactory::register_<OclPostProcessBlender>(YAMI_VPP_OCL_BLENDER);

}

