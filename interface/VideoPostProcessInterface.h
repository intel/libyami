/*
 *  VideoPostProcessInterface.h- Post Process API
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

#ifndef VIDEO_POST_PROCESS_INTERFACE_H_
#define VIDEO_POST_PROCESS_INTERFACE_H_

#include "VideoCommonDefs.h"
#include "VideoPostProcessDefs.h"

namespace YamiMediaCodec{
/**
 * \class IVideoPostProcess
 * \brief Abstract video post process interface of libyami
 * it is the interface with client. they are not thread safe.
 */
class IVideoPostProcess {
  public:
    // set native display
    virtual YamiStatus  setNativeDisplay(const NativeDisplay& display) = 0;
    // it may return YAMI_MORE_DATA when output is not ready
    // it's no need fill every field of VideoFrame, we can get detailed information from lower level
    // VideoFrame.surface is enough for most of case
    // for some type of vpp such as deinterlace, we will hold a referece of src.
    virtual YamiStatus process(const SharedPtr<VideoFrame>& src,
                               const SharedPtr<VideoFrame>& dest) = 0;
    virtual YamiStatus setParameters(VppParamType type, void* vppParam) = 0;
    virtual ~IVideoPostProcess() {}
};
}
#endif                          /* VIDEO_POST_PROCESS_INTERFACE_H_ */
