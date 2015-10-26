/*
 *  oclpostprocess_blend.h - ocl based alpha blending
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
#ifndef oclpostprocess_blender_h
#define oclpostprocess_blender_h


#include "interface/VideoCommonDefs.h"
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

private:
    static const bool s_registered; // VaapiPostProcessFactory registration result
};

}
#endif //oclpostprocess_blend_h