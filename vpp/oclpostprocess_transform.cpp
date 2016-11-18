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

#include "oclpostprocess_transform.h"
#include "common/common_def.h"
#include "common/log.h"
#include "ocl/oclcontext.h"
#include "vaapi/VaapiUtils.h"
#include "vpp/oclvppimage.h"

namespace YamiMediaCodec {

YamiStatus
OclPostProcessTransform::process(const SharedPtr<VideoFrame>& src,
    const SharedPtr<VideoFrame>& dst)
{
    YamiStatus status = ensureContext("transform");
    if (status != YAMI_SUCCESS)
        return status;

    if (src->fourcc != YAMI_FOURCC_NV12 || dst->fourcc != YAMI_FOURCC_NV12) {
        ERROR("only support transform of NV12 video frame");
        return YAMI_INVALID_PARAM;
    }

    cl_image_format format;
    format.image_channel_order = CL_RGBA;
    format.image_channel_data_type = CL_UNORM_INT8;
    SharedPtr<OclVppCLImage> srcImage = OclVppCLImage::create(m_display, src, m_context, format);
    if (!srcImage) {
        ERROR("failed to create cl image from src video frame");
        return YAMI_FAIL;
    }
    SharedPtr<OclVppCLImage> dstImage = OclVppCLImage::create(m_display, dst, m_context, format);
    if (!dstImage) {
        ERROR("failed to create cl image from dst video frame");
        return YAMI_FAIL;
    }

    if (m_transform & VPP_TRANSFORM_FLIP_H && m_transform & VPP_TRANSFORM_FLIP_V) {
        // flip both v and h is effectively rotate 180
        m_transform &= ~(VPP_TRANSFORM_FLIP_H | VPP_TRANSFORM_FLIP_V);
        switch (m_transform) {
        case VPP_TRANSFORM_NONE:
            m_transform = VPP_TRANSFORM_ROT_180;
        case VPP_TRANSFORM_ROT_90:
            m_transform = VPP_TRANSFORM_ROT_270;
            break;
        case VPP_TRANSFORM_ROT_180:
            m_transform = VPP_TRANSFORM_NONE;
            break;
        case VPP_TRANSFORM_ROT_270:
            m_transform = VPP_TRANSFORM_ROT_90;
            break;
        default:
            ERROR("unsupported transform type");
            return YAMI_INVALID_PARAM;
        }
    }

    status = YAMI_INVALID_PARAM;
    switch (m_transform) {
    case VPP_TRANSFORM_FLIP_H | VPP_TRANSFORM_ROT_90:
    case VPP_TRANSFORM_FLIP_V | VPP_TRANSFORM_ROT_270:
        status = flipAndRotate(srcImage, dstImage, m_kernelFlipHRot90);
        break;
    case VPP_TRANSFORM_FLIP_V | VPP_TRANSFORM_ROT_90:
    case VPP_TRANSFORM_FLIP_H | VPP_TRANSFORM_ROT_270:
        status = flipAndRotate(srcImage, dstImage, m_kernelFlipVRot90);
        break;
    case VPP_TRANSFORM_FLIP_H | VPP_TRANSFORM_ROT_180:
    case VPP_TRANSFORM_FLIP_V:
        m_transform = VPP_TRANSFORM_FLIP_V;
        status = flip(srcImage, dstImage);
        break;
    case VPP_TRANSFORM_FLIP_V | VPP_TRANSFORM_ROT_180:
    case VPP_TRANSFORM_FLIP_H:
        m_transform = VPP_TRANSFORM_FLIP_H;
        status = flip(srcImage, dstImage);
        break;
    case VPP_TRANSFORM_ROT_90:
    case VPP_TRANSFORM_ROT_180:
    case VPP_TRANSFORM_ROT_270:
        status = rotate(srcImage, dstImage);
        break;
    default:
        ERROR("unsupported transform type");
        break;
    }

    return status;
}

YamiStatus OclPostProcessTransform::setParameters(VppParamType type, void* vppParam)
{
    YamiStatus status = YAMI_INVALID_PARAM;

    switch (type) {
    case VppParamTypeTransform: {
        VppParamTransform* param = (VppParamTransform*)vppParam;
        if (param->size == sizeof(VppParamTransform)) {
            m_transform = param->transform;
            status = YAMI_SUCCESS;
        }
    } break;
    default:
        status = OclPostProcessBase::setParameters(type, vppParam);
        break;
    }
    return status;
}

YamiStatus OclPostProcessTransform::flip(const SharedPtr<OclVppCLImage>& src,
    const SharedPtr<OclVppCLImage>& dst)
{
    uint32_t width = src->getWidth();
    uint32_t height = src->getHeight();
    if (width != dst->getWidth() || height != dst->getHeight()) {
        ERROR("flip failed due to unmatched resolution");
        return YAMI_INVALID_PARAM;
    }

    uint32_t size;
    cl_kernel kernel = NULL;
    if (m_transform & VPP_TRANSFORM_FLIP_H) {
        size = width / 4 - 1;
        kernel = m_kernelFlipH;
    }
    else if (m_transform & VPP_TRANSFORM_FLIP_V) {
        size = height;
        kernel = m_kernelFlipV;
    }
    if (!kernel) {
        ERROR("failed to get cl kernel");
        return YAMI_FAIL;
    }
    if ((CL_SUCCESS != clSetKernelArg(kernel, 0, sizeof(cl_mem), &dst->plane(0)))
         || (CL_SUCCESS != clSetKernelArg(kernel, 1, sizeof(cl_mem), &dst->plane(1)))
         || (CL_SUCCESS != clSetKernelArg(kernel, 2, sizeof(cl_mem), &src->plane(0)))
         || (CL_SUCCESS != clSetKernelArg(kernel, 3, sizeof(cl_mem), &src->plane(1)))
         || (CL_SUCCESS != clSetKernelArg(kernel, 4, sizeof(uint32_t), &size))) {
        ERROR("clSetKernelArg failed");
        return YAMI_FAIL;
    }

    size_t globalWorkSize[2], localWorkSize[2];
    localWorkSize[0] = 8;
    localWorkSize[1] = 8;
    globalWorkSize[0] = ALIGN_POW2(width, localWorkSize[0] * 4) / 4;
    globalWorkSize[1] = ALIGN_POW2(height, localWorkSize[1] * 2) / 2;
    if (!checkOclStatus(clEnqueueNDRangeKernel(m_context->m_queue, kernel, 2, NULL,
                            globalWorkSize, localWorkSize, 0, NULL, NULL),
            "EnqueueNDRangeKernel")) {
        return YAMI_FAIL;
    }
    return YAMI_SUCCESS;
}

YamiStatus OclPostProcessTransform::rotate(const SharedPtr<OclVppCLImage>& src,
    const SharedPtr<OclVppCLImage>& dst)
{
    uint32_t width = src->getWidth();
    uint32_t height = src->getHeight();

    uint32_t size, w, h;
    cl_kernel kernel = NULL;
    if (m_transform & VPP_TRANSFORM_ROT_90) {
        if (width != dst->getHeight() || height != dst->getWidth()) {
            ERROR("rotate failed due to unmatched resolution");
            return YAMI_INVALID_PARAM;
        }
        size = 4;
        w = width / 4 - 1;
        h = height / 4 - 1;
        kernel = m_kernelRot90;
    }
    else if (m_transform & VPP_TRANSFORM_ROT_180) {
        if (width != dst->getWidth() || height != dst->getHeight()) {
            ERROR("rotate failed due to unmatched resolution");
            return YAMI_INVALID_PARAM;
        }
        size = 2;
        w = width / 4 - 1;
        h = height;
        kernel = m_kernelRot180;
    }
    else if (m_transform & VPP_TRANSFORM_ROT_270) {
        if (width != dst->getHeight() || height != dst->getWidth()) {
            ERROR("rotate failed due to unmatched resolution");
            return YAMI_INVALID_PARAM;
        }
        size = 4;
        w = width / 4 - 1;
        h = height / 4 - 1;
        kernel = m_kernelRot270;
    }
    if (!kernel) {
        ERROR("failed to get cl kernel");
        return YAMI_FAIL;
    }
    if ((CL_SUCCESS != clSetKernelArg(kernel, 0, sizeof(cl_mem), &dst->plane(0)))
         || (CL_SUCCESS != clSetKernelArg(kernel, 1, sizeof(cl_mem), &dst->plane(1)))
         || (CL_SUCCESS != clSetKernelArg(kernel, 2, sizeof(cl_mem), &src->plane(0)))
         || (CL_SUCCESS != clSetKernelArg(kernel, 3, sizeof(cl_mem), &src->plane(1)))
         || (CL_SUCCESS != clSetKernelArg(kernel, 4, sizeof(uint32_t), &w))
         || (CL_SUCCESS != clSetKernelArg(kernel, 5, sizeof(uint32_t), &h))) {
        ERROR("clSetKernelArg failed");
        return YAMI_FAIL;
    }

    size_t globalWorkSize[2], localWorkSize[2];
    localWorkSize[0] = 8;
    localWorkSize[1] = 8;
    globalWorkSize[0] = ALIGN_POW2(width, localWorkSize[0] * 4) / 4;
    globalWorkSize[1] = ALIGN_POW2(height, localWorkSize[1] * size) / size;
    if (!checkOclStatus(clEnqueueNDRangeKernel(m_context->m_queue, kernel, 2, NULL,
                            globalWorkSize, localWorkSize, 0, NULL, NULL),
            "EnqueueNDRangeKernel")) {
        return YAMI_FAIL;
    }
    return YAMI_SUCCESS;
}

YamiStatus OclPostProcessTransform::flipAndRotate(const SharedPtr<OclVppCLImage>& src,
    const SharedPtr<OclVppCLImage>& dst,
    const cl_kernel& kernel)
{
    uint32_t width = src->getWidth();
    uint32_t height = src->getHeight();
    if (width != dst->getHeight() || height != dst->getWidth()) {
        ERROR("flipAndRotate failed due to unmatched resolution");
        return YAMI_INVALID_PARAM;
    }

    uint32_t size = 4;
    uint32_t w = width / 4 - 1;
    uint32_t h = height / 4 - 1;
    if (!kernel) {
        ERROR("failed to get cl kernel");
        return YAMI_FAIL;
    }
    if ((CL_SUCCESS != clSetKernelArg(kernel, 0, sizeof(cl_mem), &dst->plane(0)))
         || (CL_SUCCESS != clSetKernelArg(kernel, 1, sizeof(cl_mem), &dst->plane(1)))
         || (CL_SUCCESS != clSetKernelArg(kernel, 2, sizeof(cl_mem), &src->plane(0)))
         || (CL_SUCCESS != clSetKernelArg(kernel, 3, sizeof(cl_mem), &src->plane(1)))
         || (CL_SUCCESS != clSetKernelArg(kernel, 4, sizeof(uint32_t), &w))
         || (CL_SUCCESS != clSetKernelArg(kernel, 5, sizeof(uint32_t), &h))) {
        ERROR("clSetKernelArg failed");
        return YAMI_FAIL;
    }

    size_t globalWorkSize[2], localWorkSize[2];
    localWorkSize[0] = 8;
    localWorkSize[1] = 8;
    globalWorkSize[0] = ALIGN_POW2(width, localWorkSize[0] * 4) / 4;
    globalWorkSize[1] = ALIGN_POW2(height, localWorkSize[1] * size) / size;
    if (!checkOclStatus(clEnqueueNDRangeKernel(m_context->m_queue, kernel, 2, NULL,
                            globalWorkSize, localWorkSize, 0, NULL, NULL),
            "EnqueueNDRangeKernel")) {
        return YAMI_FAIL;
    }
    return YAMI_SUCCESS;
}

bool OclPostProcessTransform::prepareKernels()
{
    m_kernelFlipH = prepareKernel("transform_flip_h");
    m_kernelFlipV = prepareKernel("transform_flip_v");
    m_kernelRot180 = prepareKernel("transform_rot_180");
    m_kernelRot90 = prepareKernel("transform_rot_90");
    m_kernelRot270 = prepareKernel("transform_rot_270");
    m_kernelFlipHRot90 = prepareKernel("transform_flip_h_rot_90");
    m_kernelFlipVRot90 = prepareKernel("transform_flip_v_rot_90");

    return (m_kernelFlipH != NULL)
        && (m_kernelFlipV != NULL)
        && (m_kernelRot180 != NULL)
        && (m_kernelRot90 != NULL)
        && (m_kernelRot270 != NULL)
        && (m_kernelFlipHRot90 != NULL)
        && (m_kernelFlipVRot90 != NULL);
}

}
