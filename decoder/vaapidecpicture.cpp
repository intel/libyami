/*
 *  vaapidecpicture.cpp - base picture for decoder
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
#include "vaapidecpicture.h"

#include "log.h"

VaapiDecPicture::VaapiDecPicture(VADisplay display, VAContextID context,
                                 const SurfacePtr& surface, int64_t timeStamp)
    :VaapiPicture(display, context, surface, timeStamp)
{
}

bool VaapiDecPicture::decode()
{
    return VaapiPicture::render();
}

bool VaapiDecPicture::doRender()
{
    RENDER_OBJECT(m_picture);
    RENDER_OBJECT(m_probTable);
    RENDER_OBJECT(m_iqMatrix);
    RENDER_OBJECT(m_bitPlane);
    RENDER_OBJECT(m_hufTable);
    if (!render(m_slices)) {
        ERROR("render m_slices failed");
        return false;
    }
    return true;
}

