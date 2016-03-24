/*
 *  vaapipostprocess_scaler.h- base class for video post process
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

#ifndef vaapipostprocess_scaler_h
#define vaapipostprocess_scaler_h

#include "vaapipostprocess_base.h"
#include <va/va.h>

namespace YamiMediaCodec{

typedef struct {
    VABufferID pipeline_param_buffer_id;
    VABufferID denoise_filter_buffer_id;
    VABufferID deinterlace_filter_buffer_id;
    VABufferID sharp_filter_buffer_id;
    VAProcPipelineParameterBuffer pipeline_param;
    VAProcFilterParameterBuffer denoise_param;
    VAProcFilterParameterBufferDeinterlacing deint_param;
    VAProcFilterParameterBuffer sharp_param;
    VABufferID vpp_filters[VAProcFilterCount];
    int num_vpp_filter_used;
} VPPContext;

/* class for video scale and color space conversion */
class VaapiPostProcessScaler : public VaapiPostProcessBase {
public:
    VaapiPostProcessScaler();
    YamiStatus setParameters(VppParamType type, void* vppParam);
    virtual YamiStatus process(const SharedPtr<VideoFrame>& src,
                               const SharedPtr<VideoFrame>& dest);

private:
    static const bool s_registered; // VaapiPostProcessFactory registration result
    VPPFilterParameters m_vppParam;
    VPPContext m_vppContext;

    bool EnableDeinterlace(VaapiVppPicture *picture);
    bool EnableDenoise(VaapiVppPicture *picture);
    bool EnableSharp(VaapiVppPicture *picture);
};
}
#endif                          /* vaapipostprocess_scaler_h */
