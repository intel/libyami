/*
 *  oclpostprocess_mosaic.h - opencl based mosaic filter
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
#ifndef oclpostprocess_mosaic_h
#define oclpostprocess_mosaic_h

#include "interface/VideoCommonDefs.h"
#include "oclpostprocess_base.h"

namespace YamiMediaCodec {

/**
 * \class OclPostProcessMosaic
 * \brief OpenCL based mosaic filter
 */
class OclPostProcessMosaic : public OclPostProcessBase {
public:
    virtual YamiStatus process(const SharedPtr<VideoFrame>& src,
        const SharedPtr<VideoFrame>& dst);

    virtual YamiStatus setParameters(VppParamType type, void* vppParam);

    explicit OclPostProcessMosaic()
        : m_blockSize(32)
    {
    }

private:
    static const bool s_registered; // VaapiPostProcessFactory registration result
    int m_blockSize;
};
}
#endif //oclpostprocess_mosaic_h
