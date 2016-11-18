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
#ifndef oclpostprocess_osd_h
#define oclpostprocess_osd_h

#include <vector>
#include "VideoCommonDefs.h"
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
        ensureContext("osd");
    }

private:
    friend class FactoryTest<IVideoPostProcess, OclPostProcessOsd>;
    friend class OclPostProcessOsdTest;

    virtual bool prepareKernels();
    YamiStatus computeBlockLuma(const SharedPtr<VideoFrame> frame);

    /**
     * VaapiPostProcessFactory registration result. This postprocess is
     * registered in vaapipostprocess_host.cpp
     */
    static const bool s_registered;

    vector<float> m_osdLuma;
    vector<uint32_t> m_lineBuf;
    int m_blockCount;
    uint32_t m_threshold;
    cl_kernel m_kernelOsd;
    cl_kernel m_kernelReduceLuma;
};
}
#endif //oclpostprocess_osd_h
