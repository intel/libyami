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

#ifndef vaapipic_h
#define vaapipic_h

#include "vaapibuffer.h"
#include "vaapiptrs.h"
#include "vaapisurface.h"
#include "vaapitypes.h"
#include <string.h>
#include <va/va.h>
#include <vector>

//fixme: change name to VaapiPicture when we decoder/vaapipicture.h removed

class VaapiPic
{
public:
    VaapiPic(VADisplay display, VAContextID context,const SurfacePtr& surface, int64_t timeStamp);
    virtual ~VaapiPic() {}

    VaapiPictureType        m_type;
    int64_t                 m_timeStamp;

protected:
    virtual bool doRender() = 0;

    bool render();
    bool render(const BufObjectPtr& buffer);
    template<class T>
    bool editMember(BufObjectPtr& member , VABufferType, T*& bufPtr);

    template<class T>
    BufObjectPtr createBufferObject(VABufferType, T*& bufPtr);

    inline BufObjectPtr createBufferObject(VABufferType bufType,
                                           uint32_t size,const void *data, void **mapped_data);

private:
    VADisplay              m_display;
    VAContextID            m_context;

    SurfacePtr           m_surface;
};

template<class T>
BufObjectPtr VaapiPic::createBufferObject(VABufferType  bufType, T*& bufPtr)
{
    BufObjectPtr  p = createBufferObject(bufType, sizeof(T), NULL, (void**)&bufPtr);
    if (p)
        memset(bufPtr, 0, sizeof(T));
    return p;
}

BufObjectPtr VaapiPic::createBufferObject(VABufferType bufType,
                                          uint32_t size,const void *data, void **mapped_data)
{
    return VaapiBufObject::create(m_display, m_context, bufType, size, data, mapped_data);
}

template<class T>
bool VaapiPic::editMember(BufObjectPtr& member , VABufferType bufType, T*& bufPtr)
{
    /* already set*/
    if (member)
        return false;
    member = createBufferObject(bufType, bufPtr);
    return member != NULL;
}

#define RENDER_OBJECT(mem) \
do { if (mem && !VaapiPic::render(mem)) { ERROR("render " #mem " failed"); return false;} } while(0)


template <class P, class O>
bool render(P picture, std::vector<O>& objects)
{
    for (int i = 0; i < objects.size(); i++) {
        if (!picture->render(objects[i]))
            return false;
    }
    return true;
}

#endif //vaapipic_h
