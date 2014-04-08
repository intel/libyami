/*
 *  vaapipicture.c - objects for va decode
 *
 *  Copyright (C) 2013 Intel Corporation
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
#include "common/vaapiutils.h"
#include <assert.h>

/* picture class implementation */
VaapiPicture::VaapiPicture(VADisplay display,
                           VAContextID context,
                           VaapiSurfaceBufferPool * surfBufPool,
                           VaapiPictureStructure structure)
:


m_display(display), m_context(context),
m_surfBufPool(surfBufPool), m_picStructure(structure)
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

    if (m_surfBufPool) {
        m_surfBuf = m_surfBufPool->acquireFreeBufferWithWait();
        if (m_surfBuf) {
            m_surfaceID = m_surfBuf->renderBuffer.surface;
        } else
            ERROR("VaapiPicture: acquire surface fail");
    }
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
    //m_sliceArray.clear();

    // XXX, has the surface been rendered?
    if (m_surfBufPool && m_surfBuf) {
        m_surfBufPool->recycleBuffer(m_surfBuf, false);
        m_surfBuf = NULL;
    }

}

void VaapiPicture::addSlice(VaapiSlice * slice)
{
    m_sliceArray.push_back(slice);
}

VaapiSlice *VaapiPicture::getLastSlice()
{
    return m_sliceArray.back();
}

bool VaapiPicture::decodePicture()
{
    VAStatus status;
    uint32_t i;
    vector < VaapiSlice * >::iterator iter;
    VABufferID bufferID;

    DEBUG("VP: decode picture 0x%08x", m_surfaceID);

    status = vaBeginPicture(m_display, m_context, m_surfaceID);
    if (!checkVaapiStatus(status, "vaBeginPicture()"))
        return false;

    if (m_picParam) {
        bufferID = m_picParam->getID();
        status = vaRenderPicture(m_display, m_context, &bufferID, 1);
        if (!checkVaapiStatus
            (status, "vaRenderPicture(), render pic param"))
            return false;
    }

    if (m_iqMatrix) {
        bufferID = m_iqMatrix->getID();
        status = vaRenderPicture(m_display, m_context, &bufferID, 1);
        if (!checkVaapiStatus
            (status, "vaRenderPicture(), render IQ matrix"))
            return false;
    }

    if (m_probTable) {
        bufferID = m_probTable->getID();
        status = vaRenderPicture(m_display, m_context, &bufferID, 1);
        if (!checkVaapiStatus
            (status, "vaRenderPicture(), render probability table"))
            return false;
    }

    if (m_bitPlane) {
        bufferID = m_bitPlane->getID();
        status = vaRenderPicture(m_display, m_context, &bufferID, 1);
        if (!checkVaapiStatus
            (status, "vaRenderPicture(), render bit plane"))
            return false;
    }

    if (m_hufTable) {
        bufferID = m_hufTable->getID();
        status = vaRenderPicture(m_display, m_context, &bufferID, 1);
        if (!checkVaapiStatus
            (status, "vaRenderPicture(), render huf table"))
            return false;
    }

    for (iter = m_sliceArray.begin(); iter != m_sliceArray.end(); iter++) {
        VaapiBufObject *paramBuf = (*iter)->m_param;
        VaapiBufObject *dataBuf = (*iter)->m_data;
        VABufferID vaBuffers[2];

        vaBuffers[0] = paramBuf->getID();
        vaBuffers[1] = dataBuf->getID();

        status = vaRenderPicture(m_display, m_context, vaBuffers, 2);
        if (!checkVaapiStatus(status, "vaRenderPicture()"))
            return false;
    }

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
