/*
 * Copyright (C) 2014 Intel Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "vaapicodedbuffer.h"

#include "vaapi/vaapicontext.h"
#include <string.h>

namespace YamiMediaCodec{
CodedBufferPtr VaapiCodedBuffer::create(const ContextPtr& context, uint32_t bufSize)
{
    CodedBufferPtr coded;
    BufObjectPtr buf = VaapiBuffer::create(context, VAEncCodedBufferType, bufSize);
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
}
