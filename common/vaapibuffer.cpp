/*
 *  vaapicodecobject.cpp - Basic object used for decode/encode
 *
 *  Copyright (C) 2013 Intel Corporation
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
                               uint32_t bufType, void *data, uint32_t size)
:m_display(display), m_size(size), m_buf(NULL)
{
    VAStatus status;

    if (size == 0) {
        ERROR("buffer size is zero");
        return;
    }

    if (!vaapiCreateBuffer(display,
                           context,
                           bufType, size, data, &m_bufID, (void **) 0)) {
        ERROR("create buffer failed");
        return;
    }
}

VaapiBufObject::~VaapiBufObject()
{
    if (m_buf) {
        vaapiUnmapBuffer(m_display, m_bufID, &m_buf);
        m_buf = NULL;
    }

    vaapiDestroyBuffer(m_display, &m_bufID);
}

VABufferID VaapiBufObject::getID()
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
