/*
 * Copyright (C) 2015-2016 Intel Corporation. All rights reserved.
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
#ifndef oclpostprocess_blender_h
#define oclpostprocess_blender_h


#include "VideoCommonDefs.h"
#include "oclpostprocess_base.h"

namespace YamiMediaCodec{

/**
 * \class OclPostProcessBlend
 * \brief alpha blending base on opencl
 */
class OclPostProcessBlender : public OclPostProcessBase {
public:
    virtual YamiStatus process(const SharedPtr<VideoFrame>& src,
                               const SharedPtr<VideoFrame>& dest);
    explicit OclPostProcessBlender()
        : m_kernelBlend(NULL)
    {
        ensureContext("blend");
    }

private:
    friend class FactoryTest<IVideoPostProcess, OclPostProcessBlender>;
    friend class OclPostProcessBlenderTest;

    virtual bool prepareKernels();
    YamiStatus blend(const SharedPtr<VideoFrame>& src,
                     const SharedPtr<VideoFrame>& dst);

    /**
     * VaapiPostProcessFactory registration result. This postprocess is
     * registered in vaapipostprocess_host.cpp
     */
    static const bool s_registered;

    cl_kernel m_kernelBlend;
};

}
#endif //oclpostprocess_blend_h
