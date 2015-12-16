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

#include "oclpostprocess_blender.h"
#include "vaapipostprocess_factory.h"
#include "common/common_def.h"
#include "common/log.h"
#include "ocl/oclcontext.h"

namespace YamiMediaCodec{

YamiStatus
OclPostProcessBlender::blend(const SharedPtr<VideoFrame>& src,
                             const SharedPtr<VideoFrame>& dst)
{
    YamiStatus ret = YAMI_SUCCESS;
    SharedPtr<OclImage> srcImagePtr, dstImagePtr;
    cl_mem bgImageMem[3];
    cl_image_format srcFormat, dstFormat;
    size_t globalWorkSize[2], localWorkSize[2];
    VideoRect crop;
    uint32_t pixelSize, i;

    srcFormat.image_channel_order = CL_RGBA;
    srcFormat.image_channel_data_type = CL_UNORM_INT8;
    srcImagePtr = createCLImage(src, srcFormat);
    if (!srcImagePtr) {
        ERROR("failed to create cl image from src frame");
        return YAMI_FAIL;
    }

    dstFormat.image_channel_order = CL_RG;
    dstFormat.image_channel_data_type = CL_UNORM_INT8;
    dstImagePtr = createCLImage(dst, dstFormat);
    if (!dstImagePtr) {
        ERROR("failed to create cl image from dst frame");
        return YAMI_FAIL;
    }

    if (srcImagePtr->m_format != VA_FOURCC_RGBA ||
        dstImagePtr->m_format != VA_FOURCC_NV12) {
        ERROR("only support RGBA blending on NV12");
        return YAMI_INVALID_PARAM;
    }

    for (i = 0; i < dstImagePtr->m_numPlanes; i++) {
        bgImageMem[i] = dstImagePtr->m_mem[i];
    }
    pixelSize = getPixelSize(dstFormat);
    crop.x = dst.get()->crop.x / pixelSize;
    crop.y = dst.get()->crop.y & ~1;
    crop.width = dst.get()->crop.width / pixelSize;
    crop.height = dst.get()->crop.height;
    if ((CL_SUCCESS != clSetKernelArg(m_kernel, 0, sizeof(cl_mem), &dstImagePtr->m_mem[0])) ||
        (CL_SUCCESS != clSetKernelArg(m_kernel, 1, sizeof(cl_mem), &dstImagePtr->m_mem[1])) ||
        (CL_SUCCESS != clSetKernelArg(m_kernel, 2, sizeof(cl_mem), &bgImageMem[0]))  ||
        (CL_SUCCESS != clSetKernelArg(m_kernel, 3, sizeof(cl_mem), &bgImageMem[1]))  ||
        (CL_SUCCESS != clSetKernelArg(m_kernel, 4, sizeof(cl_mem), &srcImagePtr->m_mem[0])) ||
        (CL_SUCCESS != clSetKernelArg(m_kernel, 5, sizeof(uint32_t), &crop.x))   ||
        (CL_SUCCESS != clSetKernelArg(m_kernel, 6, sizeof(uint32_t), &crop.y))   ||
        (CL_SUCCESS != clSetKernelArg(m_kernel, 7, sizeof(uint32_t), &crop.width)) ||
        (CL_SUCCESS != clSetKernelArg(m_kernel, 8, sizeof(uint32_t), &crop.height))) {
        ERROR("clSetKernelArg failed");
        return YAMI_FAIL;
    }

    // each work group has 8x8 work items; each work item handles 2x2 pixels
    localWorkSize[0] = 8;
    localWorkSize[1] = 8;
    globalWorkSize[0] = ALIGN16(dst.get()->crop.width) / pixelSize;
    globalWorkSize[1] = ALIGN16(dst.get()->crop.height) / 2;
    if (!checkOclStatus(clEnqueueNDRangeKernel(m_context->m_queue, m_kernel, 2, NULL,
        globalWorkSize, localWorkSize, 0, NULL, NULL), "EnqueueNDRangeKernel")) {
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

    return blend(src, dest);
}

const bool OclPostProcessBlender::s_registered =
    VaapiPostProcessFactory::register_<OclPostProcessBlender>(YAMI_VPP_OCL_BLENDER);

}

