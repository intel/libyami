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

#ifndef vaapipostprocess_base_h
#define vaapipostprocess_base_h

#include "VideoPostProcessInterface.h"
#include "vaapi/vaapiptrs.h"
#include <va/va.h>
#include <va/va_vpp.h>

template <class B, class C>
class FactoryTest;

namespace YamiMediaCodec{
/**
 * \class IVideoPostProcess
 * \brief Abstract video post process interface of libyami
 * it is the interface with client. they are not thread safe.
 */
class VaapiPostProcessBase : public IVideoPostProcess {
public:
    // set native display
    virtual YamiStatus  setNativeDisplay(const NativeDisplay& display);
    // it may return YAMI_MORE_DATA when output is not ready
    // it's no need fill every field of VideoFrame, we can get detailed information from lower level
    // VideoFrame.surface is enough for most of case
    // for some type of vpp such as deinterlace, we will hold a referece of src.
    virtual YamiStatus process(const SharedPtr<VideoFrame>& src,
                               const SharedPtr<VideoFrame>& dest) = 0;
    virtual YamiStatus setParameters(VppParamType type, void* vppParam) { return YAMI_INVALID_PARAM; }
    virtual ~VaapiPostProcessBase();
protected:
    //NativeDisplay   m_externalDisplay;
    YamiStatus initVA(const NativeDisplay& display);
    void cleanupVA();

    YamiStatus queryVideoProcFilterCaps(VAProcFilterType filterType, void* filterCaps, uint32_t* numFilterCaps = NULL);

    DisplayPtr m_display;
    ContextPtr m_context;
};
}
#endif                          /* vaapipostprocess_base_h */
