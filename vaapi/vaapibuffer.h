/*
 *  vaapibuffer.h just abstract for VABuffer
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
#ifndef vaapibuffer_h
#define vaapibuffer_h

#include "interface/VideoCommonDefs.h"
#include "vaapiptrs.h"

#include <va/va.h>
#include <stdint.h>

namespace YamiMediaCodec{

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
    DISALLOW_COPY_AND_ASSIGN(VaapiBuffer)
};
}

#endif //vaapibuffer_h
