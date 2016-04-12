/*
 * Copyright 2016 Intel Corporation
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

#ifndef VaapiBuffer_h
#define VaapiBuffer_h

#include "common/NonCopyable.h"
#include "vaapiptrs.h"

#include <va/va.h>
#include <stdint.h>

namespace YamiMediaCodec {

class VaapiBuffer {
public:
    static BufObjectPtr create(const ContextPtr&,
        VABufferType,
        uint32_t size,
        const void* data = 0);
    void* map();
    void unmap();
    uint32_t getSize();
    VABufferID getID();
    ~VaapiBuffer();

private:
    VaapiBuffer(const DisplayPtr&, VABufferID id, uint32_t size);
    DisplayPtr m_display;
    VABufferID m_id;
    void* m_data;
    uint32_t m_size;
    DISALLOW_COPY_AND_ASSIGN(VaapiBuffer);
};

}

#endif //VaapiBuffer_h
