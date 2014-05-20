/*
 *  vaapiencpicture.cpp - va encoder picture
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
#include "vaapiencpicture.h"

#include "log.h"
#include "vaapibuffer.h"
#include "vaapisurface.h"
#include <stdlib.h>


using std::vector;
using std::tr1::shared_ptr;
typedef VaapiEncPicture::PackedHeaderPtr PackedHeaderPtr;

class VaapiEncPackedHeader
{
    friend class VaapiEncPicture;
public:
    static PackedHeaderPtr create(VADisplay, VAContextID, VAEncPackedHeaderType, const void* header, uint32_t headerBitSize);
private:
    VaapiEncPackedHeader(const BufObjectPtr& param, const BufObjectPtr& data);
    BufObjectPtr m_param;
    BufObjectPtr m_data;
};

PackedHeaderPtr VaapiEncPackedHeader::create(VADisplay display, VAContextID context, VAEncPackedHeaderType headerType, const void* header, uint32_t headerBitSize)
{
    PackedHeaderPtr h;

    VAEncPackedHeaderParameterBuffer packedHeader;
    packedHeader.type = headerType;
    packedHeader.bit_length = headerBitSize;
    packedHeader.has_emulation_bytes = 0;

    BufObjectPtr param = VaapiBufObject::create(display, context, VAEncPackedHeaderParameterBufferType, sizeof(packedHeader), &packedHeader);
    BufObjectPtr data = VaapiBufObject::create(display, context, VAEncPackedHeaderDataBufferType, (headerBitSize + 7)/8, header);
    if (param && data)
        h.reset(new VaapiEncPackedHeader(param, data));
    return h;
}

VaapiEncPackedHeader::VaapiEncPackedHeader(const BufObjectPtr& param, const BufObjectPtr& data):
    m_param(param),
    m_data(data)
{
}

VaapiEncPicture::VaapiEncPicture(VADisplay display, VAContextID context, const SurfacePtr& surface, int64_t timeStamp)
    :m_display(display),
     m_context(context),
     m_surface(surface),
     m_timeStamp(timeStamp),
     m_type(VAAPI_PICTURE_TYPE_NONE)
{
}

bool VaapiEncPicture::encode(const BufObjectPtr& object)
{
    if (!object) {
        ERROR("bug: no object to encode");
        return false;
    }

    if (object->isMapped())
        object->unmap();

    VAStatus status;
    VABufferID id = object->getID();
    status = vaRenderPicture(m_display, m_context, &id, 1);
    if (!checkVaapiStatus (status, "vaRenderPicture()"))
        return false;
    return true;
}

bool VaapiEncPicture::sync()
{
    m_surface->sync();
}

bool VaapiEncPicture::encode(const PackedHeaderPtr& header)
{
    if (!header) {
        ERROR("bug: no packed header to encode");
        return false;
    }
    if (!encode(header->m_param))
        return false;
    if (!encode(header->m_data))
        return false;
    return true;
}

template<class T>
bool VaapiEncPicture::encode(vector< shared_ptr<T> >& v)
{
    for (typename vector< shared_ptr<T> >::iterator it = v.begin(); it != v.end(); ++it) {
        if (!encode(*it))
            return false;
    }
    return true;
}

bool VaapiEncPicture::doEncode()
{
    if (m_sequence && !encode(m_sequence)) {
        ERROR("render sequence failed");
        return false;
    }
    if (!encode(m_packedHeaders)) {
        ERROR("render packed headers failed");
        return false;
    }
    if (!encode(m_miscParams)) {
        ERROR("render misc failed");
        return false;
    }
    if (!encode(m_picture)) {
        ERROR("render picture header failed");
        return false;
    }
    if (!encode(m_slices)) {
        ERROR("render slices failed");
        return false;
    }
    return true;
}

bool VaapiEncPicture::encode()
{
    if (m_surface->getID() == VA_INVALID_SURFACE) {
        ERROR("bug: no surface to encode");
        return false;
    }

    VAStatus status;
    status = vaBeginPicture(m_display, m_context, m_surface->getID());
    if (!checkVaapiStatus (status, "vaBeginPicture()"))
        return false;

    bool ret = doEncode();

    status = vaEndPicture(m_display, m_context);
    if (!checkVaapiStatus (status, "vaEndPicture()"))
        return false;
    return ret;
}

bool VaapiEncPicture::addPackedHeader(VAEncPackedHeaderType packedHeaderType, const void* data, uint32_t dataBitSize)
{
    PackedHeaderPtr header = VaapiEncPackedHeader::create(m_display, m_context, packedHeaderType, data,dataBitSize);
    if (!header)
        return false;
    m_packedHeaders.push_back(header);
    return true;
}

BufObjectPtr VaapiEncPicture::createBufferObject(VABufferType  bufType, int size, void** bufPtr)
{
    BufObjectPtr obj =  VaapiBufObject::create(m_display,m_context,bufType,size, NULL, bufPtr);
    if (obj)
        memset(*bufPtr, 0, size);
    return obj;
}

