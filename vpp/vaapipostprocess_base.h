/*
 *  vaapipostprocess_base.h- base class for video post process
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
#include "vaapi/vaapiptrs.h"

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
    virtual ~VaapiPostProcessBase();
protected:
    //NativeDisplay   m_externalDisplay;
    YamiStatus initVA(const NativeDisplay& display);
    void cleanupVA();

    DisplayPtr m_display;
    ContextPtr m_context;
};
}
#endif                          /* vaapipostprocess_base_h */
