/*
 *  vaapivpppicture.h- base class for video post process
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

#ifndef vaapivpppicture_h
#define vaapivpppicture_h

#include "vaapi/vaapipicture.h"
#include <va/va_vpp.h>

namespace YamiMediaCodec{

class VaapiVppPicture:public VaapiPicture {
public:
    VaapiVppPicture(const ContextPtr& context,
                    const SurfacePtr & surface);
    virtual ~VaapiVppPicture() { }

    bool editVppParam(VAProcPipelineParameterBuffer*&);

    bool process();

protected:
    bool doRender();
private:
    BufObjectPtr m_vppParam;
    DISALLOW_COPY_AND_ASSIGN(VaapiVppPicture);
};

}

#endif                          /* vaapivpppicture_h */
