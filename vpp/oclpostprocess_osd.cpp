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

#include "oclpostprocess_osd.h"
#include "common/common_def.h"
#include "common/log.h"
#include "ocl/oclcontext.h"
#include "vpp/oclvppimage.h"

namespace YamiMediaCodec {

YamiStatus
OclPostProcessOsd::process(const SharedPtr<VideoFrame>& src,
    const SharedPtr<VideoFrame>& dst)
{
    YamiStatus status = ensureContext("osd");
    if (status != YAMI_SUCCESS)
        return status;

    if (src->fourcc != YAMI_FOURCC_RGBA || dst->fourcc != YAMI_FOURCC_NV12) {
        ERROR("only support RGBA OSD on NV12 video frame");
        return YAMI_INVALID_PARAM;
    }

    status = computeBlockLuma(dst);
    if (status != YAMI_SUCCESS)
        return status;

    cl_int clStatus;
    cl_mem clBuf = clCreateBuffer(m_context->m_context,
        CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY,
        m_osdLuma.size() * sizeof(float),
        NULL,
        &clStatus);
    SharedPtr<cl_mem> osdLuma(new cl_mem(clBuf), OclMemDeleter());
    if (!checkOclStatus(clStatus, "CreateBuffer")
        || !checkOclStatus(clEnqueueWriteBuffer(m_context->m_queue, *osdLuma, CL_TRUE, 0,
                                                    m_osdLuma.size() * sizeof(float),
                                                    m_osdLuma.data(), 0, NULL, NULL),
                               "EnqueueWriteBuffer")) {
        return YAMI_FAIL;
    }

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
    if ((CL_SUCCESS != clSetKernelArg(m_kernelOsd, 0, sizeof(cl_mem), &dstImagePtr->plane(0)))
        || (CL_SUCCESS != clSetKernelArg(m_kernelOsd, 1, sizeof(cl_mem), &dstImagePtr->plane(1)))
        || (CL_SUCCESS != clSetKernelArg(m_kernelOsd, 2, sizeof(cl_mem), &bgImageMem[0]))
        || (CL_SUCCESS != clSetKernelArg(m_kernelOsd, 3, sizeof(cl_mem), &bgImageMem[1]))
        || (CL_SUCCESS != clSetKernelArg(m_kernelOsd, 4, sizeof(cl_mem), &srcImagePtr->plane(0)))
        || (CL_SUCCESS != clSetKernelArg(m_kernelOsd, 5, sizeof(uint32_t), &crop.x))
        || (CL_SUCCESS != clSetKernelArg(m_kernelOsd, 6, sizeof(uint32_t), &crop.y))
        || (CL_SUCCESS != clSetKernelArg(m_kernelOsd, 7, sizeof(uint32_t), &crop.width))
        || (CL_SUCCESS != clSetKernelArg(m_kernelOsd, 8, sizeof(uint32_t), &crop.height))
        || (CL_SUCCESS != clSetKernelArg(m_kernelOsd, 9, sizeof(cl_mem), osdLuma.get()))) {
        ERROR("clSetKernelArg failed");
        return YAMI_FAIL;
    }

    size_t globalWorkSize[2], localWorkSize[2];
    localWorkSize[0] = 8;
    localWorkSize[1] = 8;
    globalWorkSize[0] = ALIGN_POW2(dst->crop.width, localWorkSize[0] * pixelSize) / pixelSize;
    globalWorkSize[1] = ALIGN_POW2(dst->crop.height, localWorkSize[1] * 2) / 2;
    if (!checkOclStatus(clEnqueueNDRangeKernel(m_context->m_queue, m_kernelOsd, 2, NULL,
                            globalWorkSize, localWorkSize, 0, NULL, NULL),
            "EnqueueNDRangeKernel")) {
        return YAMI_FAIL;
    }

    return status;
}

YamiStatus OclPostProcessOsd::setParameters(VppParamType type, void* vppParam)
{
    YamiStatus status = YAMI_INVALID_PARAM;

    switch (type) {
    case VppParamTypeOsd: {
        VppParamOsd* osd = (VppParamOsd*)vppParam;
        if (osd->size == sizeof(VppParamOsd)) {
            m_threshold = osd->threshold;
            status = YAMI_SUCCESS;
        }
    } break;
    default:
        status = OclPostProcessBase::setParameters(type, vppParam);
        break;
    }
    return status;
}

YamiStatus OclPostProcessOsd::computeBlockLuma(const SharedPtr<VideoFrame> frame)
{
    uint32_t blockWidth = frame->crop.height;
    cl_image_format format;
    format.image_channel_order = CL_RGBA;
    format.image_channel_data_type = CL_UNSIGNED_INT8;
    uint32_t pixelSize = getPixelSize(format);

    if (m_blockCount < (int)(frame->crop.width / blockWidth)) {
        m_blockCount = frame->crop.width / blockWidth;
        m_osdLuma.resize(m_blockCount);
    }

    uint32_t padding = frame->crop.x % pixelSize;
    uint32_t alignedWidth = frame->crop.width + padding;
    if (m_lineBuf.size() < alignedWidth)
        m_lineBuf.resize(alignedWidth);

    cl_int clStatus;
    cl_mem clBuf = clCreateBuffer(m_context->m_context,
        CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY,
        m_lineBuf.size() * sizeof(uint32_t),
        NULL,
        &clStatus);
    if (!checkOclStatus(clStatus, "CreateBuffer"))
        return YAMI_FAIL;
    SharedPtr<cl_mem> lineBuf(new cl_mem(clBuf), OclMemDeleter());

    SharedPtr<OclVppCLImage> imagePtr;
    imagePtr = OclVppCLImage::create(m_display, frame, m_context, format);
    if (!imagePtr) {
        ERROR("failed to create cl image from src frame");
        return YAMI_FAIL;
    }

    VideoRect crop;
    crop.x = frame->crop.x / pixelSize;
    crop.y = frame->crop.y;
    crop.width = alignedWidth / pixelSize;
    crop.height = frame->crop.height;
    if ((CL_SUCCESS != clSetKernelArg(m_kernelReduceLuma, 0, sizeof(cl_mem), &imagePtr->plane(0)))
        || (CL_SUCCESS != clSetKernelArg(m_kernelReduceLuma, 1, sizeof(uint32_t), &crop.x))
        || (CL_SUCCESS != clSetKernelArg(m_kernelReduceLuma, 2, sizeof(uint32_t), &crop.y))
        || (CL_SUCCESS != clSetKernelArg(m_kernelReduceLuma, 3, sizeof(uint32_t), &crop.height))
        || (CL_SUCCESS != clSetKernelArg(m_kernelReduceLuma, 4, sizeof(cl_mem), lineBuf.get()))) {
        ERROR("clSetKernelArg failed");
        return YAMI_FAIL;
    }
    size_t localWorkSize = 16;
    size_t globalWorkSize = ALIGN_POW2(alignedWidth, pixelSize * localWorkSize) / pixelSize;
    if (!checkOclStatus(clEnqueueNDRangeKernel(m_context->m_queue, m_kernelReduceLuma, 1, NULL,
                            &globalWorkSize, &localWorkSize, 0, NULL, NULL),
            "EnqueueNDRangeKernel")) {
        return YAMI_FAIL;
    }
    if (!checkOclStatus(clEnqueueReadBuffer(m_context->m_queue,
                            *lineBuf,
                            CL_TRUE,
                            0,
                            m_lineBuf.size() * sizeof(uint32_t),
                            m_lineBuf.data(),
                            0, NULL, NULL),
            "EnqueueReadBuffer")) {
        return YAMI_FAIL;
    }

    uint32_t acc;
    int offset;
    uint32_t blockThreshold = m_threshold * blockWidth * frame->crop.height;
    for (int i = 0; i < m_blockCount; i++) {
        acc = 0;
        offset = i * blockWidth + padding;
        for (uint32_t j = 0; j < blockWidth; j++) {
            acc += m_lineBuf[offset + j];
        }
        if (acc <= blockThreshold)
            m_osdLuma[i] = 1.0;
        else
            m_osdLuma[i] = 0.0;
    }

    return YAMI_SUCCESS;
}

bool OclPostProcessOsd::prepareKernels()
{
    m_kernelOsd = prepareKernel("osd");
    m_kernelReduceLuma = prepareKernel("reduce_luma");

    return (m_kernelOsd != NULL)
        && (m_kernelReduceLuma != NULL);
}

}
