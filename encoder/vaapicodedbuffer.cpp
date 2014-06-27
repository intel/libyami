/*
 *  vaapicodedbuffer.cpp - codedbuffer wrapper for va
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
#include "vaapicodedbuffer.h"

#include "vaapicontext.h"
#include <string.h>

CodedBufferPtr VaapiCodedBuffer::create(const ContextPtr& context, uint32_t bufSize)
{
    CodedBufferPtr coded;
    BufObjectPtr buf = VaapiBufObject::create(context, VAEncCodedBufferType, bufSize);
    if (buf)
        coded.reset(new VaapiCodedBuffer(buf));
    return coded;
}

bool VaapiCodedBuffer::map()
{
    if (!m_segments)
        m_segments = static_cast<VACodedBufferSegment*>(m_buf->map());
    return m_segments != NULL;
}

uint32_t VaapiCodedBuffer::size()
{
    if (!map())
        return 0;
    uint32_t size = 0;
    VACodedBufferSegment* segment = m_segments;
    while (segment != NULL) {
        size += segment->size;
        segment = static_cast<VACodedBufferSegment*>(segment->next);
    }
    return size;
}

bool VaapiCodedBuffer::copyInto(void* data)
{
    if (!data)
        return false;
    if (!map())
        return false;
    uint8_t* dest = static_cast<uint8_t*>(data);
    VACodedBufferSegment* segment = m_segments;
    while (segment != NULL) {
        memcpy(dest, segment->buf, segment->size);
        dest += segment->size;
        segment = static_cast<VACodedBufferSegment*>(segment->next);
    }
    return true;
}
