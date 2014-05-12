/*
 *  vaapipicture.c - objects for va decode
 *
 *  Copyright (C) 2010-2011 Splitted-Desktop Systems
 *    Author: Gwenole Beauchesne <gwenole.beauchesne@splitted-desktop.com>
 *  Copyright (C) 2011-2014 Intel Corporation
 *    Author: Gwenole Beauchesne <gwenole.beauchesne@intel.com>
 *    Author: Xiaowei Li <xiaowei.li@intel.com>
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

#include "vaapipicture.h"
#include "log.h"
#include "vaapi/vaapiutils.h"
#include <assert.h>

/* picture class implementation */
VaapiPicture::VaapiPicture(VADisplay display,
                           VAContextID context,
                           VaapiSurfaceBufferPool * surfBufPool,
                           VaapiPictureStructure structure)
:  m_display(display)
    , m_context(context)
    , m_surfBufPool(surfBufPool)
    , m_picStructure(structure)
{
    m_picParam = NULL;
    m_iqMatrix = NULL;
    m_bitPlane = NULL;
    m_hufTable = NULL;
    m_probTable = NULL;

    m_timeStamp = INVALID_PTS;
    m_POC = INVALID_POC;
    m_flags = 0;
    m_surfBuf = NULL;
    m_surfaceID = 0;

    if (!m_surfBuf && m_surfBufPool) {
        m_surfBuf = m_surfBufPool->acquireFreeBufferWithWait();
    }

    if (m_surfBuf)
       m_surfaceID = m_surfBuf->renderBuffer.surface;
}

VaapiPicture::~VaapiPicture()
{
    vector < VaapiSlice * >::iterator iter;

    if (m_picParam) {
        delete m_picParam;
        m_picParam = NULL;
    }

    if (m_iqMatrix) {
        delete m_iqMatrix;
        m_iqMatrix = NULL;
    }

    if (m_bitPlane) {
        delete m_bitPlane;
        m_bitPlane = NULL;
    }

    if (m_hufTable) {
        delete m_hufTable;
        m_hufTable = NULL;
    }

    if (m_probTable) {
        delete m_probTable;
        m_probTable = NULL;
    }

    for (iter = m_sliceArray.begin(); iter != m_sliceArray.end(); iter++)
        delete *iter;

    if (m_surfBufPool && m_surfBuf) {
        m_surfBufPool->recycleBuffer(m_surfBuf, false);
        m_surfBuf = NULL;
    }

}

void VaapiPicture::attachSurfaceBuf(VaapiSurfaceBufferPool * surfBufPool,
                                    VideoSurfaceBuffer *surfBuf,
                                    VaapiPictureStructure structure)
{
    if (m_surfBuf && m_surfBufPool) {
       m_surfBufPool->recycleBuffer(m_surfBuf, false);
       m_surfaceID = 0;
    }

    m_surfBufPool  = surfBufPool;
    m_surfBuf      = surfBuf;
    m_picStructure = structure;

    if (m_surfBuf)
       m_surfaceID = m_surfBuf->renderBuffer.surface;
}


void VaapiPicture::addSlice(VaapiSlice * slice)
{
    m_sliceArray.push_back(slice);
}

VaapiSlice *VaapiPicture::getLastSlice()
{
    return m_sliceArray.back();
}

bool VaapiPicture::renderVaBuffer(VaapiBufObject * &buffer,
                                  const char *bufferInfo)
{
    VAStatus status = VA_STATUS_SUCCESS;
    VABufferID bufferID = VA_INVALID_ID;

    if (!buffer)
        return false;

    bufferID = buffer->getID();
    if (bufferID == VA_INVALID_ID)
        return false;

    status = vaRenderPicture(m_display, m_context, &bufferID, 1);
    if (!checkVaapiStatus(status, bufferInfo))
        return false;

    delete buffer;
    buffer = NULL;

    return true;
}

bool VaapiPicture::decodePicture()
{
    VAStatus status;
    uint32_t i;
    vector < VaapiSlice * >::iterator iter;

    DEBUG("VaapiPicture::decodePicture 0x%08x", m_surfaceID);

    status = vaBeginPicture(m_display, m_context, m_surfaceID);
    if (!checkVaapiStatus(status, "vaBeginPicture()"))
        return false;

    if (m_picParam) {
        if (!renderVaBuffer
            (m_picParam, "vaRenderPicture(), render pic param"))
            return false;
    }

    if (m_probTable) {
        if (!renderVaBuffer
            (m_probTable, "vaRenderPicture(), render probability table"))
            return false;
    }

    if (m_iqMatrix) {
        if (!renderVaBuffer
            (m_iqMatrix, "vaRenderPicture(), render IQ matrix"))
            return false;
    }

    if (m_bitPlane) {
        if (!renderVaBuffer
            (m_bitPlane, "vaRenderPicture(), render bit plane"))
            return false;
    }

    if (m_hufTable) {
        if (!renderVaBuffer
            (m_hufTable, "vaRenderPicture(), render huffman table"))
            return false;
    }

    for (iter = m_sliceArray.begin(); iter != m_sliceArray.end(); ++iter) {
        VaapiBufObject *paramBuf = (*iter)->m_param;
        VaapiBufObject *dataBuf = (*iter)->m_data;

        if (!renderVaBuffer
            (paramBuf, "vaRenderPicture(), render slice param"))
            break;

        if (!renderVaBuffer
            (dataBuf, "vaRenderPicture(), render slice data"))
            break;
    }

    if (iter != m_sliceArray.end()) {
        m_sliceArray.clear();
        return false;
    }
    m_sliceArray.clear();

    status = vaEndPicture(m_display, m_context);
    if (!checkVaapiStatus(status, "vaEndPicture()"))
        return false;
    return true;
}

bool VaapiPicture::output()
{
    bool isReferenceFrame = false;
    bool usedAsReference = false;

    if (!m_surfBufPool || !m_surfBuf) {
        ERROR("no surface buffer pool ");
        return false;
    }

    if (m_type == VAAPI_PICTURE_TYPE_I || m_type == VAAPI_PICTURE_TYPE_P)
        isReferenceFrame = true;

    if (m_flags & VAAPI_PICTURE_FLAG_REFERENCE)
        usedAsReference = true;

    m_surfBufPool->setReferenceInfo(m_surfBuf, isReferenceFrame,
                                    usedAsReference);

    return m_surfBufPool->outputBuffer(m_surfBuf, m_timeStamp, m_POC);
}
