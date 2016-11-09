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
#ifndef oclpostprocess_transform_h
#define oclpostprocess_transform_h

#include <vector>
#include "VideoCommonDefs.h"
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
        , m_kernelFlipH(NULL)
        , m_kernelFlipV(NULL)
        , m_kernelRot180(NULL)
        , m_kernelRot90(NULL)
        , m_kernelRot270(NULL)
        , m_kernelFlipHRot90(NULL)
        , m_kernelFlipVRot90(NULL)
    {
        ensureContext("transform");
    }

private:
    friend class FactoryTest<IVideoPostProcess, OclPostProcessTransform>;
    friend class OclPostProcessTransformTest;

    virtual bool prepareKernels();
    YamiStatus flip(const SharedPtr<OclVppCLImage>& src,
        const SharedPtr<OclVppCLImage>& dst);
    YamiStatus rotate(const SharedPtr<OclVppCLImage>& src,
        const SharedPtr<OclVppCLImage>& dst);
    YamiStatus flipAndRotate(const SharedPtr<OclVppCLImage>& src,
        const SharedPtr<OclVppCLImage>& dst,
        const cl_kernel& kernel);

    /**
     * VaapiPostProcessFactory registration result. This postprocess is
     * registered in vaapipostprocess_host.cpp
     */
    static const bool s_registered;

    uint32_t m_transform;
    cl_kernel m_kernelFlipH;
    cl_kernel m_kernelFlipV;
    cl_kernel m_kernelRot180;
    cl_kernel m_kernelRot90;
    cl_kernel m_kernelRot270;
    cl_kernel m_kernelFlipHRot90;
    cl_kernel m_kernelFlipVRot90;
};
}
#endif //oclpostprocess_transform_h
