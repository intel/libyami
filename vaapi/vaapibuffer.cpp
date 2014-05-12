/*
 *  vaapibuffer.cpp - Basic object used for decode/encode
 *
 *  Copyright (C) 2010-2011 Splitted-Desktop Systems
 *    Author: Gwenole Beauchesne <gwenole.beauchesne@splitted-desktop.com>
 *  Copyright (C) 2011-2014 Intel Corporation
 *    Author: Gwenole Beauchesne <gwenole.beauchesne@intel.com>
 *    Author: Xiaowei Li<xiaowei.li@intel.com>
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

#include "vaapibuffer.h"
#include "log.h"
#include "vaapiutils.h"
#include <va/va.h>

VaapiBufObject::VaapiBufObject(VADisplay display,
                               VAContextID context,
                               uint32_t bufType, const void *data,
                               uint32_t size)
:m_display(display), m_size(size), m_buf(NULL)
{
    VAStatus status;

    if (size == 0) {
        ERROR("buffer size is zero");
        return;
    }

    if (!vaapiCreateBuffer(display, context,
                           bufType, size, data, &m_bufID, (void **) 0)) {
        ERROR("create buffer failed");
        return;
    }
}

VaapiBufObject::VaapiBufObject(VADisplay display,
                               VABufferID bufID, void *buf, uint32_t size)
:m_display(display), m_bufID(bufID), m_buf(buf), m_size(size)
{

}

VaapiBufObject::~VaapiBufObject()
{
    if (m_buf) {
        vaapiUnmapBuffer(m_display, m_bufID, &m_buf);
        m_buf = NULL;
    }

    vaapiDestroyBuffer(m_display, &m_bufID);
}

VABufferID VaapiBufObject::getID() const
{
    return m_bufID;
}

uint32_t VaapiBufObject::getSize()
{
    return m_size;
}

void *VaapiBufObject::map()
{
    if (m_buf)
        return m_buf;

    m_buf = vaapiMapBuffer(m_display, m_bufID);
    return m_buf;
}

void VaapiBufObject::unmap()
{
    if (m_buf) {
        vaapiUnmapBuffer(m_display, m_bufID, &m_buf);
        m_buf = NULL;
    }
}

bool VaapiBufObject::isMapped() const
{
    return m_buf != NULL;
}

BufObjectPtr VaapiBufObject::create(VADisplay display,
                                    VAContextID context,
                                    VABufferType bufType,
                                    uint32_t size,
                                    const void *data, void **mapped_data)
{
    VAStatus status;
    BufObjectPtr buf;

    if (size == 0) {
        ERROR("buffer size is zero");
        return buf;
    }

    VABufferID bufID;
    if (!vaapiCreateBuffer(display, context,
                           bufType, size, data, &bufID, mapped_data)) {
        ERROR("create buffer failed");
        return buf;
    }

    void *mapped = mapped_data ? *mapped_data : NULL;
    buf.reset(new VaapiBufObject(display, bufID, mapped, size));
    return buf;
}
