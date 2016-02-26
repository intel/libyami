/*
 *  oclpostprocess_base.h- base class for opencl based video post process
 *
 *  Copyright (C) 2015 Intel Corporation
 *    Author: XuGuangxin<Guangxin.Xu@intel.com>
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

#ifndef vaapipostprocess_base_h
#define vaapipostprocess_base_h

#include "VideoPostProcessInterface.h"
#include <CL/opencl.h>
#include <ocl/oclcontext.h>
#include <va/va.h>

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
    cl_kernel getKernel(const char* name);

    VADisplay m_display;
    OclKernelMap m_kernels;
    SharedPtr<OclContext> m_context;
};
}
#endif                          /* vaapipostprocess_base_h */

