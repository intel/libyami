/*
 *  oclpostprocess_osd.h - opencl based OSD of contrastive font color
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
#ifndef oclpostprocess_osd_h
#define oclpostprocess_osd_h

#include <vector>
#include "interface/VideoCommonDefs.h"
#include "oclpostprocess_base.h"

using std::vector;

namespace YamiMediaCodec {

/**
 * \class OclPostProcessOsd
 * \brief OpenCL based OSD of contrastive font color
 */
class OclPostProcessOsd : public OclPostProcessBase {
public:
    virtual YamiStatus process(const SharedPtr<VideoFrame>& src,
        const SharedPtr<VideoFrame>& dst);

    virtual YamiStatus setParameters(VppParamType type, void* vppParam);

    explicit OclPostProcessOsd()
        : m_blockCount(0)
        , m_threshold(128)
        , m_kernelOsd(NULL)
        , m_kernelReduceLuma(NULL)
    {
    }

private:
    virtual bool prepareKernels();
    YamiStatus computeBlockLuma(const SharedPtr<VideoFrame> frame);

    static const bool s_registered; // VaapiPostProcessFactory registration result
    vector<float> m_osdLuma;
    vector<uint32_t> m_lineBuf;
    int m_blockCount;
    uint32_t m_threshold;
    cl_kernel m_kernelOsd;
    cl_kernel m_kernelReduceLuma;
};
}
#endif //oclpostprocess_osd_h
