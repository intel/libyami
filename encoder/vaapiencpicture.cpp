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

VaapiEncPicture::VaapiEncPicture(VADisplay display, VAContextID context,
                                 const SurfacePtr & surface,
                                 int64_t timeStamp)
:VaapiPicture(display, context, surface, timeStamp)
{
}

bool VaapiEncPicture::encode()
{
    return render();
}

bool VaapiEncPicture::doRender()
{
    RENDER_OBJECT(m_sequence);
    RENDER_OBJECT(m_packedHeaders);
    RENDER_OBJECT(m_miscParams);
    RENDER_OBJECT(m_picture);
    RENDER_OBJECT(m_slices);
    return true;
}

bool VaapiEncPicture::
addPackedHeader(VAEncPackedHeaderType packedHeaderType, const void *header,
                uint32_t headerBitSize)
{
    VAEncPackedHeaderParameterBuffer *packedHeader;
    BufObjectPtr param =
        createBufferObject(VAEncPackedHeaderParameterBufferType,
                           packedHeader);
    BufObjectPtr data =
        createBufferObject(VAEncPackedHeaderDataBufferType,
                           (headerBitSize + 7) / 8, header, NULL);
    bool ret = addObject(m_packedHeaders, param, data);
    if (ret) {
        packedHeader->type = packedHeaderType;
        packedHeader->bit_length = headerBitSize;
        packedHeader->has_emulation_bytes = 0;
    }
    return ret;
}
