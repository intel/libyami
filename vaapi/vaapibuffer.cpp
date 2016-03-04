/*
 *  vaapibuffer.cpp abstract for VABuffer
 *
 *  Copyright (C) 2016 Intel Corporation
 *    Author: Xu Guangxin <Guangxin.Xu@intel.com>
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

#include "vaapiutils.h"
#include "vaapicontext.h"
#include "vaapidisplay.h"

namespace YamiMediaCodec{

BufObjectPtr VaapiBuffer::create(const ContextPtr& context,
    VABufferType type,
    uint32_t size,
    const void* data)
{
    BufObjectPtr buf;
    if (!context || !context->getDisplay())
        return buf;
    DisplayPtr display = context->getDisplay();
    VABufferID id;
    VAStatus status = vaCreateBuffer(display->getID(), context->getID(),
        type, size, 1, (void*)data, &id);
    if (!checkVaapiStatus(status, "vaCreateBuffer"))
        return buf;
    buf.reset(new VaapiBuffer(display, id, size));
    return buf;
}

void* VaapiBuffer::map()
{
    if (!m_data) {
        VAStatus status = vaMapBuffer(m_display->getID(), m_id, &m_data);
        if (!checkVaapiStatus(status, "vaMapBuffer")) {
            m_data = NULL;
        }
    }
    return m_data;
}

void VaapiBuffer::unmap()
{
    if (m_data) {
        checkVaapiStatus(vaUnmapBuffer(m_display->getID(), m_id), "vaUnmapBuffer");
        m_data = NULL;
    }
}

uint32_t VaapiBuffer::getSize()
{
    return m_size;
}

VABufferID VaapiBuffer::getID()
{
    return m_id;
}

VaapiBuffer::VaapiBuffer(const DisplayPtr& display, VABufferID id, uint32_t size)
    : m_display(display)
    , m_id(id)
    , m_data(NULL)
    , m_size(size)
{
}

VaapiBuffer::~VaapiBuffer()
{
    unmap();
    checkVaapiStatus(vaDestroyBuffer(m_display->getID(), m_id), "vaDestroyBuffer");
}

}
