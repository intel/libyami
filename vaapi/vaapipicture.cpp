/*
 *  vaapipicture.c - base picture for va decoder and encoder
 *
 *  Copyright (C) 2014 Intel Corporation
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
#include "vaapipicture.h"

#include "common/log.h"
#include "vaapibuffer.h"
#include "vaapidisplay.h"
#include "vaapicontext.h"
#include "vaapisurface.h"
#include "vaapi/vaapiutils.h"

namespace YamiMediaCodec{
VaapiPicture::VaapiPicture(const ContextPtr& context,
                           const SurfacePtr& surface, int64_t timeStamp)
:m_display(context->getDisplay()), m_context(context), m_surface(surface),
m_timeStamp(timeStamp), m_type(VAAPI_PICTURE_TYPE_NONE)
{

}

VaapiPicture::VaapiPicture()
:m_timeStamp(0), m_type(VAAPI_PICTURE_TYPE_NONE)
{
}

bool VaapiPicture::render()
{
    if (m_surface->getID() == VA_INVALID_SURFACE) {
        ERROR("bug: no surface to encode");
        return false;
    }

    VAStatus status;
    status = vaBeginPicture(m_display->getID(), m_context->getID(), m_surface->getID());
    if (!checkVaapiStatus(status, "vaBeginPicture()"))
        return false;

    bool ret = doRender();

    status = vaEndPicture(m_display->getID(), m_context->getID());
    if (!checkVaapiStatus(status, "vaEndPicture()"))
        return false;
    return ret;
}

bool VaapiPicture::render(BufObjectPtr& buffer)
{
    VAStatus status = VA_STATUS_SUCCESS;
    VABufferID bufferID = VA_INVALID_ID;

    if (!buffer)
        return true;

    buffer->unmap();

    bufferID = buffer->getID();
    if (bufferID == VA_INVALID_ID)
        return false;

    status = vaRenderPicture(m_display->getID(), m_context->getID(), &bufferID, 1);
    if (!checkVaapiStatus(status, "vaRenderPicture failed"))
        return false;

    buffer.reset();             // silently work  arouond for psb
    return true;
}

bool VaapiPicture::render(std::pair <BufObjectPtr,BufObjectPtr> &paramAndData)
{
    return render(paramAndData.first) && render(paramAndData.second);
}

bool VaapiPicture::addObject(std::vector<std::pair<BufObjectPtr,BufObjectPtr> >& objects,
                             const BufObjectPtr & param,
                             const BufObjectPtr & data)
{
    if (!param || !data)
        return false;
    objects.push_back(std::make_pair(param, data));
    return true;
}

bool VaapiPicture::addObject(std::vector < BufObjectPtr >& objects,
                             const BufObjectPtr& object)
{
    if (!object)
        return false;
    objects.push_back(object);
    return true;
}

bool VaapiPicture::sync()
{
    return vaSyncSurface(m_display->getID(), getSurfaceID()) == VA_STATUS_SUCCESS;
}
}
