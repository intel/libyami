/*
 *  vaapiencpicture.h - va encoder picture
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

#ifndef vaapiencpicture_h
#define vaapiencpicture_h

#include "vaapibuffer.h"
#include "vaapiptrs.h"
#include "vaapisurface.h"
#include "vaapitypes.h"
#include <string.h>
#include <va/va.h>
#include <vector>


class VaapiEncPackedHeader;
class VaapiEncPicture
{
public:
    typedef std::tr1::shared_ptr<VaapiEncPackedHeader> PackedHeaderPtr;
    VaapiEncPicture(VADisplay display, VAContextID context,const SurfacePtr& surface, int64_t timeStamp);
    virtual ~VaapiEncPicture() {};

    template <class T>
    bool editSequence(T*& seqParam);

    template <class T>
    bool editPicture(T*& picParam);

    template <class T>
    bool newSlice(T*& sliceParam);

    template <class T>
    bool newMisc(VAEncMiscParameterType, T*& miscParam);

    bool addPackedHeader(VAEncPackedHeaderType, const void* data, uint32_t dataBitSize);

    bool encode();
    bool sync();

    VaapiPictureType        m_type;
    int64_t                 m_timeStamp;

private:
    template<class T>
    BufObjectPtr createMiscObject(VAEncMiscParameterType, T*& bufPtr);

    template<class T>
    BufObjectPtr createBufferObject(VABufferType, T*& bufPtr);
    BufObjectPtr createBufferObject(VABufferType, int size, void** bufPtr);

    template<class T>
    bool editMember(BufObjectPtr& member , VABufferType bufType, T*& bufPtr);

    bool doEncode();
    bool encode(const BufObjectPtr &);
    bool encode(const PackedHeaderPtr &);
    template<typename T>
    bool encode(std::vector< std::tr1::shared_ptr<T> >&);


    VADisplay              m_display;
    VAContextID            m_context;

    SurfacePtr           m_surface;
    BufObjectPtr            m_sequence;
    BufObjectPtr            m_picture;
    std::vector< PackedHeaderPtr >    m_packedHeaders;
    std::vector< BufObjectPtr >    m_miscParams;
    std::vector< BufObjectPtr >    m_slices;
};

template<class T>
bool VaapiEncPicture::editSequence(T*& seqParam)
{
    return editMember(m_sequence, VAEncSequenceParameterBufferType, seqParam);
}

template<class T>
bool VaapiEncPicture::editPicture(T*& picParam)
{
    return editMember(m_picture, VAEncPictureParameterBufferType, picParam);
}

template <class T>
bool VaapiEncPicture::newSlice(T*& sliceParam)
{
    BufObjectPtr slice = createBufferObject(VAEncSliceParameterBufferType, sliceParam);
    if (slice)
        m_slices.push_back(slice);
    return slice != NULL;
}

template<class T>
BufObjectPtr VaapiEncPicture::createMiscObject(VAEncMiscParameterType  miscType, T*& bufPtr)
{
    VAEncMiscParameterBuffer* misc;
    int size = sizeof(VAEncMiscParameterBuffer) + sizeof(T);
    BufObjectPtr obj =  createBufferObject(VAEncMiscParameterBufferType, size, (void**)&misc);
    if (obj) {
        misc->type = miscType;
        bufPtr = (T*)(misc->data);
    }
    return obj;
}

template <class T>
bool VaapiEncPicture::newMisc(VAEncMiscParameterType  miscType, T*& miscParam)
{
    BufObjectPtr misc = createMiscObject(miscType, miscParam);
    if (misc)
        m_miscParams.push_back(misc);
    return misc != NULL;
}

template<class T>
BufObjectPtr VaapiEncPicture::createBufferObject(VABufferType  bufType, T*& bufPtr)
{
    return  createBufferObject(bufType, sizeof(T), (void**)&bufPtr);
}

template<class T>
bool VaapiEncPicture::editMember(BufObjectPtr& member , VABufferType bufType, T*& bufPtr)
{
    /* already set*/
    if (member)
        return false;
    member = createBufferObject(bufType, bufPtr);
    return member != NULL;
}

#endif //vaapiencpicture_h
