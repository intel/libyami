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

#ifndef vaapipicture_h
#define vaapipicture_h

#include "VideoCommonDefs.h"
#include "VaapiBuffer.h"
#include "vaapiptrs.h"
#include "VaapiSurface.h"
#include <string.h>
#include <va/va.h>
#include <vector>
#include <utility>

namespace YamiMediaCodec{

typedef enum
{
    VAAPI_PICTURE_INVALID = 0x0000,
    VAAPI_PICTURE_TOP_FIELD = 0x0001,
    VAAPI_PICTURE_BOTTOM_FIELD = 0x0002,
    VAAPI_PICTURE_FRAME = 0x0003,
    VAAPI_PICTURE_I = 0x0100,
    VAAPI_PICTURE_B = 0x0200,
    VAAPI_PICTURE_P = 0x0400
} VaapiPictureType;

class VaapiPicture
{
protected:
    DisplayPtr             m_display;
    ContextPtr             m_context;
    SurfacePtr             m_surface;

public:
    VaapiPicture(const ContextPtr&, const SurfacePtr&, int64_t timeStamp);
    virtual ~VaapiPicture() {};

    inline VASurfaceID getSurfaceID() const;
    inline SurfacePtr getSurface() const;
    inline void setSurface(const SurfacePtr&);
    bool sync();

    int64_t                 m_timeStamp;
    VaapiPictureType        m_type;

protected:
    virtual bool doRender() = 0;

    bool render();
    bool render(BufObjectPtr& buffer);
    bool render(std::pair<BufObjectPtr, BufObjectPtr>& paramAndData);

    template <class O>
    bool render(std::vector<O>& objects);

    template<class T>
    bool editObject(BufObjectPtr& object , VABufferType, T*& bufPtr);
    bool addObject(std::vector<std::pair<BufObjectPtr, BufObjectPtr> >& objects,
                   const BufObjectPtr& param, const BufObjectPtr& data);
    bool addObject(std::vector<BufObjectPtr>& objects, const BufObjectPtr& object);

    template<class T>
    BufObjectPtr createBufferObject(VABufferType, T*& bufPtr);
    inline BufObjectPtr createBufferObject(VABufferType bufType,
        uint32_t size, const void* data, void** mapped);
    VaapiPicture();
};

template<class T>
BufObjectPtr VaapiPicture::createBufferObject(VABufferType  bufType, T*& bufPtr)
{
    return VaapiBuffer::create(m_context, bufType, bufPtr);
}

BufObjectPtr VaapiPicture::createBufferObject(VABufferType bufType,
    uint32_t size, const void* data, void** mapped)
{
    return VaapiBuffer::create(m_context, bufType, size, data, mapped);
}

template<class T>
bool VaapiPicture::editObject(BufObjectPtr& object , VABufferType bufType, T*& bufPtr)
{
    /* already set? It's only one time offer*/
    if (object)
        return false;
    object = createBufferObject(bufType, bufPtr);

    if (!bufPtr)
        return false;

    return bool(object);
}

#define RENDER_OBJECT(obj) \
do { if (!VaapiPicture::render(obj)) { ERROR("render " #obj " failed"); return false;} } while(0)

template <class O>
bool VaapiPicture::render(std::vector<O>& objects)
{
    bool ret = true;

    for (uint32_t i = 0; i < objects.size(); i++)
        ret &= render(objects[i]);

    objects.clear(); // slient work around for psb drv to delete VABuffer
    return ret;
}


VASurfaceID VaapiPicture::getSurfaceID() const
{
    return m_surface->getID();
}

SurfacePtr VaapiPicture::getSurface() const
{
    return m_surface;
}

void VaapiPicture::setSurface(const SurfacePtr& surface)
{
    m_surface = surface;
}

}

#endif //vaapipicture_h

