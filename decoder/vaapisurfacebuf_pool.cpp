/*
 *  vaapisurfacebuffer_pool.cpp - VA surface pool
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

#include "vaapisurfacebuf_pool.h"
#include "common/log.h"

VaapiSurfaceBufferPool::VaapiSurfaceBufferPool(VADisplay display,
                                               VideoConfigBuffer * config)
:  m_display(display)
{
    INFO("Construct the render buffer pool ");

    uint32_t i;
    uint32_t format = VA_RT_FORMAT_YUV420;

    pthread_cond_init(&m_cond, NULL);
    pthread_mutex_init(&m_lock, NULL);

    if (!config) {
        ERROR("Wrong parameter to init render buffer pool");
        return;
    }

    m_bufCount = config->surfaceNumber;
    m_bufArray =
        (VideoSurfaceBuffer **) malloc(sizeof(VideoSurfaceBuffer *) *
                                       m_bufCount);
    m_surfArray =
        (VaapiSurface **) malloc(sizeof(VaapiSurface *) * m_bufCount);

    if (config->flag & WANT_SURFACE_PROTECTION) {
        format != VA_RT_FORMAT_PROTECTED;
        INFO("Surface is protectd ");
    }

    /* allocate surface for the pool */
    m_useExtBuf = false;
    if (config->flag & USE_NATIVE_GRAPHIC_BUFFER) {
        INFO("Use native graphci buffer for decoding");
        VASurfaceAttrib surfaceAttribs[2];
        VASurfaceAttribExternalBuffers surfAttribExtBuf;

        surfaceAttribs[0].type = VASurfaceAttribMemoryType;
        surfaceAttribs[0].flags = VA_SURFACE_ATTRIB_SETTABLE;
        surfaceAttribs[0].value.type = VAGenericValueTypeInteger;
        surfaceAttribs[0].value.value.i =
            VA_SURFACE_ATTRIB_MEM_TYPE_KERNEL_DRM;

        surfAttribExtBuf.pixel_format = VA_FOURCC_NV12;
        surfAttribExtBuf.width = config->graphicBufferWidth;
        surfAttribExtBuf.height = config->graphicBufferHeight;
        surfAttribExtBuf.data_size =
            config->graphicBufferWidth * config->graphicBufferHeight * 3 /
            2;
        surfAttribExtBuf.num_planes = 2;
        surfAttribExtBuf.pitches[0] = config->graphicBufferWidth;
        surfAttribExtBuf.offsets[0] = 0;
        surfAttribExtBuf.pitches[1] = config->graphicBufferWidth / 2;
        surfAttribExtBuf.offsets[1] =
            config->graphicBufferWidth * config->graphicBufferHeight;

        surfAttribExtBuf.num_buffers = 1;
        surfAttribExtBuf.buffers =
            (unsigned long *) malloc(sizeof(unsigned long) *
                                     surfAttribExtBuf.num_buffers);
        surfAttribExtBuf.buffers[0] = 0;
        surfAttribExtBuf.flags = 0;
        surfAttribExtBuf.private_data = NULL;
        surfaceAttribs[1].type = VASurfaceAttribExternalBufferDescriptor;
        surfaceAttribs[1].flags = VA_SURFACE_ATTRIB_SETTABLE;
        surfaceAttribs[1].value.type = VAGenericValueTypePointer;
        surfaceAttribs[1].value.value.p = &surfAttribExtBuf;

        for (i = 0; i < m_bufCount; i++) {
            surfAttribExtBuf.buffers[0] =
                (unsigned long) config->graphicBufferHandler[i];
            m_surfArray[i] =
                new VaapiSurface(display, VAAPI_CHROMA_TYPE_YUV420,
                                 config->graphicBufferWidth,
                                 config->graphicBufferHeight,
                                 (void *) &surfaceAttribs, 2);

            if (!m_surfArray[i]) {
                ERROR("VaapiSurface allocation failed ");
                return;
            }
        }

        m_useExtBuf = true;

        if (surfAttribExtBuf.buffers)
            free(surfAttribExtBuf.buffers);

    } else {
        for (i = 0; i < m_bufCount; i++) {
            m_surfArray[i] = new VaapiSurface(display,
                                              VAAPI_CHROMA_TYPE_YUV420,
                                              config->width,
                                              config->height, NULL, 0);
            if (!m_surfArray[i]) {
                ERROR("VaapiSurface allocation failed ");
                return;
            }
        }
    }

    /* Wrap vaapi surfaces into VideoSurfaceBuffer pool */
    for (i = 0; i < m_bufCount; i++) {
        m_bufArray[i] = (struct VideoSurfaceBuffer *)
            malloc(sizeof(struct VideoSurfaceBuffer));
        m_bufArray[i]->pictureOrder = INVALID_POC;
        m_bufArray[i]->referenceFrame = false;
        m_bufArray[i]->asReferernce = false;    //todo fix the typo
        m_bufArray[i]->mappedData = NULL;
        m_bufArray[i]->renderBuffer.display = display;
        m_bufArray[i]->renderBuffer.surface = m_surfArray[i]->getID();
        m_bufArray[i]->renderBuffer.scanFormat = VA_FRAME_PICTURE;
        m_bufArray[i]->renderBuffer.timeStamp = INVALID_PTS;
        m_bufArray[i]->renderBuffer.flag = 0;
        m_bufArray[i]->renderBuffer.graphicBufferIndex = i;
        if (m_useExtBuf) {
            /* buffers is hold by external client, such as graphics */
            m_bufArray[i]->renderBuffer.graphicBufferHandle =
                config->graphicBufferHandler[i];
            m_bufArray[i]->renderBuffer.renderDone = false;
            m_bufArray[i]->status = SURFACE_RENDERING;
        } else {
            m_bufArray[i]->renderBuffer.graphicBufferHandle = NULL;
            m_bufArray[i]->renderBuffer.renderDone = true;
            m_bufArray[i]->status = SURFACE_FREE;
        }
    }

    m_bufMapped = false;
    if (config->flag & WANT_RAW_OUTPUT) {
        mapSurfaceBuffers();
        m_bufMapped = true;
        INFO("Surface is mapped out for raw output ");
    }
}

VaapiSurfaceBufferPool::~VaapiSurfaceBufferPool()
{
    INFO("Destruct the render buffer pool ");
    uint32_t i;

    if (m_bufMapped) {
        unmapSurfaceBuffers();
        m_bufMapped = false;
    }

    for (i = 0; i < m_bufCount; i++) {
        delete m_surfArray[i];
        m_surfArray[i] = NULL;
    }
    free(m_surfArray);
    m_surfArray = NULL;

    for (i = 0; i < m_bufCount; i++) {
        free(m_bufArray[i]);
        m_bufArray[i] = NULL;
    }
    free(m_bufArray);
    m_bufArray = NULL;

    pthread_cond_destroy(&m_cond);
    pthread_mutex_destroy(&m_lock);
}


VideoSurfaceBuffer *VaapiSurfaceBufferPool::acquireFreeBuffer()
{
    VideoSurfaceBuffer *surfBuf = searchAvailableBuffer();

    if (surfBuf) {
        pthread_mutex_lock(&m_lock);
        surfBuf->renderBuffer.driverRenderDone = true;
        surfBuf->renderBuffer.timeStamp = INVALID_PTS;
        surfBuf->pictureOrder = INVALID_POC;
        surfBuf->status = SURFACE_DECODING;
        pthread_mutex_unlock(&m_lock);
    }
    return surfBuf;
}

VideoSurfaceBuffer *VaapiSurfaceBufferPool::acquireFreeBufferWithWait()
{
    VideoSurfaceBuffer *surfBuf = searchAvailableBuffer();

    if (!surfBuf) {
        INFO("Waiting for free surface buffer");
        pthread_mutex_lock(&m_lock);
        pthread_cond_wait(&m_cond, &m_lock);
        pthread_mutex_unlock(&m_lock);
        INFO("Receive signal about a free buffer availble");
        surfBuf = searchAvailableBuffer();
        if (!surfBuf) {
            ERROR
                ("Can not get the released surface, this should not happen");
        }
    }

    if (surfBuf) {
        pthread_mutex_lock(&m_lock);
        surfBuf->renderBuffer.driverRenderDone = true;
        surfBuf->renderBuffer.timeStamp = INVALID_PTS;
        surfBuf->pictureOrder = INVALID_POC;
        surfBuf->status = SURFACE_DECODING;
        pthread_mutex_unlock(&m_lock);
    }

    return surfBuf;
}

bool VaapiSurfaceBufferPool::setReferenceInfo(VideoSurfaceBuffer * buf,
                                              bool referenceFrame,
                                              bool asReference)
{
    pthread_mutex_lock(&m_lock);
    buf->referenceFrame = referenceFrame;
    buf->asReferernce = asReference;    //to fix typo error
    pthread_mutex_unlock(&m_lock);
    return true;
}

bool VaapiSurfaceBufferPool::outputBuffer(VideoSurfaceBuffer * buf,
                                          uint64_t timeStamp, uint32_t poc)
{
    DEBUG("Pool: set surface(ID:%8x, poc:%x) to be rendered",
          buf->renderBuffer.surface, poc);

    uint32_t i = 0;
    pthread_mutex_lock(&m_lock);
    for (i = 0; i < m_bufCount; i++) {
        if (m_bufArray[i] == buf) {
            m_bufArray[i]->pictureOrder = poc;
            m_bufArray[i]->renderBuffer.timeStamp = timeStamp;
            m_bufArray[i]->status |= SURFACE_TO_RENDER;
            break;
        }
    }
    pthread_mutex_unlock(&m_lock);

    if (i == m_bufCount) {
        ERROR("Pool: outputbuffer, Error buffer pointer ");
        return false;
    }

    return true;
}

bool VaapiSurfaceBufferPool::recycleBuffer(VideoSurfaceBuffer * buf,
                                           bool isFromRender)
{
    uint32_t i;

    pthread_mutex_lock(&m_lock);
    for (i = 0; i < m_bufCount; i++) {
        if (m_bufArray[i] == buf) {
            if (isFromRender) { //release from decoder
                m_bufArray[i]->renderBuffer.renderDone = true;
                m_bufArray[i]->status &= ~SURFACE_RENDERING;
                DEBUG("Pool: recy surf(%8x) from render, status = %d",
                      buf->renderBuffer.surface, m_bufArray[i]->status);
            } else {            //release from decoder
                m_bufArray[i]->status &= ~SURFACE_DECODING;
                DEBUG("Pool: recy surf(%8x) from decoder, status = %d",
                      buf->renderBuffer.surface, m_bufArray[i]->status);
            }

            if (m_bufArray[i]->status == SURFACE_FREE) {
                pthread_cond_signal(&m_cond);
            }

            pthread_mutex_unlock(&m_lock);
            return true;
        }
    }

    pthread_mutex_unlock(&m_lock);

    ERROR("recycleBuffer: Can not find buf=%p in the pool", buf);
    return false;
}

void VaapiSurfaceBufferPool::flushPool()
{
    uint32_t i;

    pthread_mutex_lock(&m_lock);

    for (i = 0; i < m_bufCount; i++) {
        m_bufArray[i]->referenceFrame = false;
        m_bufArray[i]->asReferernce = false;
        m_bufArray[i]->status = SURFACE_FREE;
    }

    pthread_mutex_unlock(&m_lock);
}

void VaapiSurfaceBufferPool::mapSurfaceBuffers()
{
    uint32_t i;
    VideoFrameRawData *rawData;

    pthread_mutex_lock(&m_lock);
    for (i = 0; i < m_bufCount; i++) {
        VaapiSurface *surf = m_surfArray[i];
        VaapiImage *image = surf->getDerivedImage();
        VaapiImageRaw *imageRaw = image->map();

        rawData = new VideoFrameRawData;
        rawData->width = imageRaw->width;
        rawData->height = imageRaw->height;
        rawData->fourcc = imageRaw->format;
        rawData->size = imageRaw->size;
        rawData->data = imageRaw->pixels[0];

        for (i = 0; i < imageRaw->numPlanes; i++) {
            rawData->offset[i] = imageRaw->pixels[i]
                - imageRaw->pixels[0];
            rawData->pitch[i] = imageRaw->strides[i];
        }

        m_bufArray[i]->renderBuffer.rawData = rawData;
        m_bufArray[i]->mappedData = rawData;
    }

    pthread_mutex_unlock(&m_lock);
}

void VaapiSurfaceBufferPool::unmapSurfaceBuffers()
{
    uint32_t i;
    pthread_mutex_lock(&m_lock);

    for (i = 0; i < m_bufCount; i++) {
        VaapiSurface *surf = m_surfArray[i];
        VaapiImage *image = surf->getDerivedImage();

        image->unmap();

        delete m_bufArray[i]->renderBuffer.rawData;
        m_bufArray[i]->renderBuffer.rawData = NULL;
        m_bufArray[i]->mappedData = NULL;
    }

    pthread_mutex_unlock(&m_lock);

}

VideoSurfaceBuffer *VaapiSurfaceBufferPool::getOutputByMinTimeStamp()
{
    uint32_t i;
    uint64_t pts = INVALID_PTS;
    VideoSurfaceBuffer *buf = NULL;

    pthread_mutex_lock(&m_lock);
    for (i = 0; i < m_bufCount; i++) {
        if (!(m_bufArray[i]->status & SURFACE_TO_RENDER))
            continue;

        if (m_bufArray[i]->renderBuffer.timeStamp == INVALID_PTS)
            continue;

        if ((uint64_t) (m_bufArray[i]->renderBuffer.timeStamp) < pts) {
            pts = m_bufArray[i]->renderBuffer.timeStamp;
            buf = m_bufArray[i];
        }
    }

    if (buf) {
        buf->status &= (~SURFACE_TO_RENDER);
        buf->status |= SURFACE_RENDERING;

        DEBUG("Pool: found %x to display with MIN pts = %d",
              buf->renderBuffer.surface, pts);
    }

    pthread_mutex_unlock(&m_lock);

    return buf;
}

VideoSurfaceBuffer *VaapiSurfaceBufferPool::getOutputByMinPOC()
{
    uint32_t i;
    uint32_t poc = INVALID_POC;
    VideoSurfaceBuffer *buf = NULL;

    pthread_mutex_lock(&m_lock);
    for (i = 0; i < m_bufCount; i++) {
        if (!(m_bufArray[i]->status & SURFACE_TO_RENDER))
            continue;

        if (m_bufArray[i]->renderBuffer.timeStamp == INVALID_PTS ||
            m_bufArray[i]->pictureOrder == INVALID_POC)
            continue;

        if ((uint64_t) (m_bufArray[i]->pictureOrder) < poc) {
            poc = m_bufArray[i]->pictureOrder;
            buf = m_bufArray[i];
        }
    }

    if (buf) {
        buf->status &= (~SURFACE_TO_RENDER);
        buf->status |= SURFACE_RENDERING;

        DEBUG("Pool: found %x to display with MIN poc = %x",
              buf->renderBuffer.surface, poc);
    }

    pthread_mutex_unlock(&m_lock);

    return buf;
}

VaapiSurface *VaapiSurfaceBufferPool::getVaapiSurface(VideoSurfaceBuffer *
                                                      buf)
{
    uint32_t i;

    for (i = 0; i < m_bufCount; i++) {
        if (buf->renderBuffer.surface == m_surfArray[i]->getID())
            return m_surfArray[i];
    }

    return NULL;
}

VideoSurfaceBuffer
    * VaapiSurfaceBufferPool::getBufferBySurfaceID(VASurfaceID id)
{
    uint32_t i;

    for (i = 0; i < m_bufCount; i++) {
        if (m_bufArray[i]->renderBuffer.surface == id)
            return m_bufArray[i];
    }

    return NULL;
}

VideoSurfaceBuffer *VaapiSurfaceBufferPool::getBufferByHandler(void
                                                               *graphicHandler)
{
    uint32_t i;

    if (!m_useExtBuf)
        return NULL;

    for (i = 0; i < m_bufCount; i++) {
        if (m_bufArray[i]->renderBuffer.graphicBufferHandle ==
            graphicHandler)
            return m_bufArray[i];
    }

    return NULL;
}

VideoSurfaceBuffer *VaapiSurfaceBufferPool::getBufferByIndex(uint32_t
                                                             index)
{
    return m_bufArray[index];
}

VideoSurfaceBuffer *VaapiSurfaceBufferPool::searchAvailableBuffer()
{
    uint32_t i;
    VAStatus vaStatus;
    VASurfaceStatus surfStatus;
    VideoSurfaceBuffer *surfBuf = NULL;

    pthread_mutex_lock(&m_lock);

    for (i = 0; i < m_bufCount; i++) {
        surfBuf = m_bufArray[i];
        /* skip non free surface  */
        if (surfBuf->status != SURFACE_FREE)
            continue;

        if (!m_useExtBuf)
            break;

        vaStatus = vaQuerySurfaceStatus(m_display,
                                        surfBuf->renderBuffer.surface,
                                        &surfStatus);

        if ((vaStatus == VA_STATUS_SUCCESS &&
             surfStatus == VASurfaceReady))
            break;
    }

    pthread_mutex_unlock(&m_lock);

    if (i != m_bufCount)
        return m_bufArray[i];
    else
        WARNING("Can not found availabe buffer");

    return NULL;
}
