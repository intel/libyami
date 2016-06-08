/*
 *  vaapivpppicture.cpp - va video post process picture
 *
 *  Copyright (C) 2015 Intel Corporation
 *    Author: Xu Guangxin <guangxin.xu@intel.com>
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "vaapivpppicture.h"
#include "common/log.h"

namespace YamiMediaCodec{
VaapiVppPicture::VaapiVppPicture(const ContextPtr& context,
                                 const SurfacePtr & surface)
:VaapiPicture(context, surface, 0)
{
}

bool VaapiVppPicture::editVppParam(VAProcPipelineParameterBuffer*& vppParam)
{
    uint32_t size = sizeof(VAProcPipelineParameterBuffer);
    void *data = (void *)vppParam;
    m_vppParam = createBufferObject(VAProcPipelineParameterBufferType,
                                    size, 
                                    data,
                                    (void **)vppParam);	
	
    return m_vppParam ? true:false;
}

VABufferID VaapiVppPicture::editProcFilterParam(VAProcFilterParameterBuffer* &vppParam)
{
    uint32_t size = sizeof(VAProcFilterParameterBuffer);
    void *data = (void *)vppParam;

    BufObjectPtr proc = createBufferObject(VAProcFilterParameterBufferType,
                                           size, 
                                           data,
                                           (void **)vppParam);
    addObject(m_procFilterParam, proc);
 
    return proc->getID();
}

VABufferID VaapiVppPicture::editDeinterlaceParam(VAProcFilterParameterBufferDeinterlacing*& vppParam)
{
    uint32_t size = sizeof(VAProcFilterParameterBufferDeinterlacing);
    void *data = (void *)vppParam;
    m_deinterlaceParam = createBufferObject(VAProcFilterParameterBufferType,
                                            size, 
                                            data,
                                            (void **)vppParam);
 
    return m_deinterlaceParam->getID();
}

bool  VaapiVppPicture::queryProcFilter(VAProcFilterType buf_type, void *filter_caps, unsigned int *num_filter_caps)
{
    return queryProcFilterCaps(buf_type, filter_caps, num_filter_caps);
}

bool VaapiVppPicture::process()
{
    return render();
}

bool VaapiVppPicture::doRender()
{
    RENDER_OBJECT(m_vppParam);
    return true;
}

}
