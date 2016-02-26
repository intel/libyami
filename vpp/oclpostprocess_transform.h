/*
 *  oclpostprocess_transform.h - opencl based flip and rotate
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
#ifndef oclpostprocess_transform_h
#define oclpostprocess_transform_h

#include <vector>
#include "interface/VideoCommonDefs.h"
#include "oclpostprocess_base.h"

using std::vector;

namespace YamiMediaCodec {
class OclVppCLImage;
/**
 * \class OclPostProcessTransform
 * \brief OpenCL based flip and rotate
 */
class OclPostProcessTransform : public OclPostProcessBase {
public:
    virtual YamiStatus process(const SharedPtr<VideoFrame>& src,
        const SharedPtr<VideoFrame>& dst);

    virtual YamiStatus setParameters(VppParamType type, void* vppParam);

    explicit OclPostProcessTransform()
        : m_transform(0)
    {
    }

private:
    YamiStatus flip(const SharedPtr<OclVppCLImage>& src,
        const SharedPtr<OclVppCLImage>& dst);
    YamiStatus rotate(const SharedPtr<OclVppCLImage>& src,
        const SharedPtr<OclVppCLImage>& dst);
    YamiStatus flipAndRotate(const SharedPtr<OclVppCLImage>& src,
        const SharedPtr<OclVppCLImage>& dst,
        const char* kernelName);

    static const bool s_registered; // VaapiPostProcessFactory registration result
    uint32_t m_transform;
};
}
#endif //oclpostprocess_transform_h
