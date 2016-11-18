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

#ifndef oclpostprocess_wireframe_h
#define oclpostprocess_wireframe_h

#include "interface/VideoCommonDefs.h"
#include "oclpostprocess_base.h"

namespace YamiMediaCodec {

/**
 * \class OclPostProcessWireframe
 * \brief OpenCL based wireframe filter
 */
class OclPostProcessWireframe : public OclPostProcessBase {
public:
    virtual YamiStatus process(const SharedPtr<VideoFrame>& src,
        const SharedPtr<VideoFrame>& dst);

    virtual YamiStatus setParameters(VppParamType type, void* vppParam);

    explicit OclPostProcessWireframe()
        : m_borderWidth(4)
        , m_colorY(0xFF)
        , m_colorU(0)
        , m_colorV(0)
        , m_kernelWireframe(NULL)
    {
        ensureContext("wireframe");
    }

private:
    friend class FactoryTest<IVideoPostProcess, OclPostProcessWireframe>;
    friend class OclPostProcessWireframeTest;

    virtual bool prepareKernels();

    /**
     * VaapiPostProcessFactory registration result. This postprocess is
     * registered in vaapipostprocess_host.cpp
     */
    static const bool s_registered;

    uint32_t m_borderWidth;
    uint8_t m_colorY;
    uint8_t m_colorU;
    uint8_t m_colorV;
    cl_kernel m_kernelWireframe;
};
}
#endif //oclpostprocess_wireframe_h
