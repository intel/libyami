/*
 *  vaapipic.h - base picture for va decoder and encoder
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

#ifndef vaapipicture_h
#define vaapipicture_h

#include "vaapibuffer.h"
#include "vaapiptrs.h"
#include "vaapipicturetypes.h"
#include "vaapisurface.h"
#include "vaapitypes.h"
#include <string.h>
#include <va/va.h>
#include <vector>
#include <utility>

namespace YamiMediaCodec{
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
    inline bool sync();

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
                                           uint32_t size,const void *data, void **mapped_data);
    VaapiPicture();
};

template<class T>
BufObjectPtr VaapiPicture::createBufferObject(VABufferType  bufType, T*& bufPtr)
{
    BufObjectPtr  p = createBufferObject(bufType, sizeof(T), NULL, (void**)&bufPtr);
    if (p)
        memset(bufPtr, 0, sizeof(T));
    return p;
}

BufObjectPtr VaapiPicture::createBufferObject(VABufferType bufType,
                                          uint32_t size,const void *data, void **mapped_data)
{
    return VaapiBufObject::create(m_context, bufType, size, data, mapped_data);
}

template<class T>
bool VaapiPicture::editObject(BufObjectPtr& object , VABufferType bufType, T*& bufPtr)
{
    /* already set? It's only one time offer*/
    if (object)
        return false;
    object = createBufferObject(bufType, bufPtr);
    return object;
}

#define RENDER_OBJECT(obj) \
do { if (!VaapiPicture::render(obj)) { ERROR("render " #obj " failed"); return false;} } while(0)

template <class O>
bool VaapiPicture::render(std::vector<O>& objects)
{
    bool ret = true;

    for (int i = 0; i < objects.size(); i++)
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

bool VaapiPicture::sync()
{
    return m_surface->sync();
}
}

#endif //vaapipicture_h

