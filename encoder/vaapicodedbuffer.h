/*
 *  vaapicodedbuffer.h - codedbuffer wrapper for va
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
#ifndef vaapicodedbuffer_h
#define vaapicodedbuffer_h

#include "vaapi/vaapibuffer.h"
#include "vaapi/vaapiptrs.h"
#include <stdlib.h>

namespace YamiMediaCodec{
class VaapiCodedBuffer
{
public:
    static CodedBufferPtr create(const ContextPtr&, uint32_t bufSize);
    ~VaapiCodedBuffer() {}
    uint32_t size();
    VABufferID getID() const {
        return m_buf->getID();
    }
    bool copyInto(void* data);
    bool setFlag(uint32_t flag) { m_flags |= flag; return true; }
    bool clearFlag(uint32_t flag) { m_flags &= ~flag; return true; }
    uint32_t getFlags() { return m_flags; }

private:
    VaapiCodedBuffer(const BufObjectPtr& buf):m_buf(buf), m_segments(NULL), m_flags(0) {}
    bool map();
    BufObjectPtr m_buf;
    VACodedBufferSegment* m_segments;
    uint32_t m_flags;
};
}
#endif //vaapicodedbuffer_h
