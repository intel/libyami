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
#ifndef vaapicodedbuffer_h
#define vaapicodedbuffer_h

#include "vaapi/VaapiBuffer.h"
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
