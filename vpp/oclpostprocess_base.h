/*
 * Copyright (C) 2015 Intel Corporation. All rights reserved.
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

#ifndef oclpostprocess_base_h
#define oclpostprocess_base_h

#include "VideoPostProcessInterface.h"
#include <CL/opencl.h>
#include <ocl/oclcontext.h>
#include <va/va.h>

template <class B, class C>
class FactoryTest;

namespace YamiMediaCodec{
class OclContext;
/**
 * \class OclPostProcessBase
 * \brief Abstract video post process base on opencl
 *
 */
class OclPostProcessBase : public IVideoPostProcess {
public:
    // set native display
    virtual YamiStatus  setNativeDisplay(const NativeDisplay& display);

    YamiStatus ensureContext(const char* name);
    //
    virtual YamiStatus process(const SharedPtr<VideoFrame>& src,
                               const SharedPtr<VideoFrame>& dest) = 0;
    virtual YamiStatus setParameters(VppParamType type, void* vppParam)
    {
        return YAMI_SUCCESS;
    }
    OclPostProcessBase();
    virtual ~OclPostProcessBase();

protected:
    class VideoFrameDeleter {
    public:
        VideoFrameDeleter(VADisplay display)
            : m_display(display)
        {
        }
        void operator()(VideoFrame* frame)
        {
            VASurfaceID id = (VASurfaceID)frame->surface;
            vaDestroySurfaces(m_display, &id, 1);
            delete frame;
        }

    private:
        VADisplay m_display;
    };

    uint32_t getPixelSize(const cl_image_format& fmt);
    cl_kernel prepareKernel(const char* name);

    VADisplay m_display;
    SharedPtr<OclContext> m_context;
    OclKernelMap m_kernels;

private:
    virtual bool prepareKernels() = 0;
};
}
#endif                          /* vaapipostprocess_base_h */

