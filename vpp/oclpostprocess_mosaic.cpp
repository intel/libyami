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

#include "oclpostprocess_mosaic.h"
#include "common/common_def.h"
#include "common/log.h"
#include "ocl/oclcontext.h"
#include "vpp/oclvppimage.h"

namespace YamiMediaCodec {

YamiStatus
OclPostProcessMosaic::process(const SharedPtr<VideoFrame>& src,
    const SharedPtr<VideoFrame>& dst)
{
    YamiStatus status = ensureContext("mosaic");
    if (status != YAMI_SUCCESS)
        return status;

    if (src != dst) {
        ERROR("src and dst must be the same for mosaic filter");
        return YAMI_INVALID_PARAM;
    }

    if (dst->fourcc != YAMI_FOURCC_NV12) {
        ERROR("only support mosaic filter on NV12 video frame");
        return YAMI_INVALID_PARAM;
    }

    cl_image_format format;
    format.image_channel_order = CL_R;
    format.image_channel_data_type = CL_UNORM_INT8;
    SharedPtr<OclVppCLImage> imagePtr = OclVppCLImage::create(m_display, dst, m_context, format);
    if (!imagePtr->numPlanes()) {
        ERROR("failed to create cl image from dst frame");
        return YAMI_FAIL;
    }

    cl_mem bgImageMem[3];
    for (uint32_t n = 0; n < imagePtr->numPlanes(); n++) {
        bgImageMem[n] = imagePtr->plane(n);
    }

    size_t globalWorkSize[2], localWorkSize[2];
    localWorkSize[0] = 256 / m_blockSize * m_blockSize;
    localWorkSize[1] = 1;
    globalWorkSize[0] = (dst->crop.width / localWorkSize[0] + 1) * localWorkSize[0];
    globalWorkSize[1] = dst->crop.height / m_blockSize + 1;
    size_t localMemSize = localWorkSize[0] * sizeof(float);

    if ((CL_SUCCESS != clSetKernelArg(m_kernelMosaic, 0, sizeof(cl_mem), &imagePtr->plane(0)))
         || (CL_SUCCESS != clSetKernelArg(m_kernelMosaic, 1, sizeof(cl_mem), &bgImageMem[0]))
         || (CL_SUCCESS != clSetKernelArg(m_kernelMosaic, 2, sizeof(cl_mem), &imagePtr->plane(1)))
         || (CL_SUCCESS != clSetKernelArg(m_kernelMosaic, 3, sizeof(cl_mem), &bgImageMem[1]))
         || (CL_SUCCESS != clSetKernelArg(m_kernelMosaic, 4, sizeof(uint32_t), &dst->crop.x))
         || (CL_SUCCESS != clSetKernelArg(m_kernelMosaic, 5, sizeof(uint32_t), &dst->crop.y))
         || (CL_SUCCESS != clSetKernelArg(m_kernelMosaic, 6, sizeof(uint32_t), &dst->crop.width))
         || (CL_SUCCESS != clSetKernelArg(m_kernelMosaic, 7, sizeof(uint32_t), &dst->crop.height))
         || (CL_SUCCESS != clSetKernelArg(m_kernelMosaic, 8, sizeof(uint32_t), &m_blockSize))
         || (CL_SUCCESS != clSetKernelArg(m_kernelMosaic, 9, localMemSize, NULL))) {
        ERROR("clSetKernelArg failed");
        return YAMI_FAIL;
    }

    if (!checkOclStatus(clEnqueueNDRangeKernel(m_context->m_queue, m_kernelMosaic, 2, NULL,
                            globalWorkSize, localWorkSize, 0, NULL, NULL),
            "EnqueueNDRangeKernel")) {
        return YAMI_FAIL;
    }

    return status;
}

YamiStatus OclPostProcessMosaic::setParameters(VppParamType type, void* vppParam)
{
    YamiStatus status = YAMI_INVALID_PARAM;

    switch (type) {
    case VppParamTypeMosaic: {
        VppParamMosaic* mosaic = (VppParamMosaic*)vppParam;
        if (mosaic->size == sizeof(VppParamMosaic)) {
            if (mosaic->blockSize > 0 && mosaic->blockSize <= 256) {
                m_blockSize = mosaic->blockSize;
                status = YAMI_SUCCESS;
            }
        }
    } break;
    default:
        status = OclPostProcessBase::setParameters(type, vppParam);
        break;
    }
    return status;
}

bool OclPostProcessMosaic::prepareKernels()
{
    m_kernelMosaic = prepareKernel("mosaic");

    return m_kernelMosaic != NULL;
}

}
