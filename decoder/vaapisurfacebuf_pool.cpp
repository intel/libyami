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

#include "common/log.h"
#include "vaapisurfacebuf_pool.h"

VaapiSurfaceBufferPool::VaapiSurfaceBufferPool(
    VADisplay display,
    VideoConfigBuffer* config)
    :mDisplay(display)
{
    uint32_t i;
    uint32_t format = VA_RT_FORMAT_YUV420;
 
    pthread_cond_init(&mCond, NULL);
    pthread_mutex_init(&mLock, NULL);

    if (!config) {
        ERROR("Wrong parameter to init render buffer pool");
        return;
    }

    mBufCount   = config->surfaceNumber;
    mBufArray   = (VideoSurfaceBuffer**) malloc (sizeof(VideoSurfaceBuffer*) * mBufCount);
    mSurfArray  = (VaapiSurface**) malloc (sizeof(VideoSurfaceBuffer*) * mBufCount);

    if (config->flag & WANT_SURFACE_PROTECTION) {
        format != VA_RT_FORMAT_PROTECTED;
        INFO("Surface is protectd ");
    }

#if VA_CHECK_VERSION(0,34,0)
    /* allocate surface for the pool */
    VASurfaceAttributeTPI surfAttrib;
    memset(&surfAttrib, 0, sizeof(VASurfaceAttributeTPI));

    if (config->flag & USE_NATIVE_GRAPHIC_BUFFER) {
         INFO("Use external buffer for decoding");
         mUseExtBuf = true;
         surfAttrib.count        = 1;
         surfAttrib.luma_stride  = config->graphicBufferStride;
         surfAttrib.pixel_format = config->graphicBufferColorFormat;
         surfAttrib.width        = config->graphicBufferWidth; 
         surfAttrib.height       = config->graphicBufferHeight;
         surfAttrib.type         = VAExternalMemoryAndroidGrallocBuffer;
         surfAttrib.reserved[0]  = (uint32_t) config->nativeWindow;
         surfAttrib.buffers      = (uint32_t*) malloc (sizeof(uint32_t) * 1);
     }

     for (i = 0; i < mBufCount; i++) {
         if (config->flag & USE_NATIVE_GRAPHIC_BUFFER) {
            surfAttrib.buffers[0] = (uint32_t)config->graphicBufferHandler[i];
         }

         mSurfArray[i] = new VaapiSurface(display,
                                          VAAPI_CHROMA_TYPE_YUV420,
                                          surfAttrib.width,
                                          surfAttrib.height,
                                          (void*)&surfAttrib);
         if (!mSurfArray[i]) {
               ERROR("VaapiSurface allocation failed ");
               return;
          }
     }

    if (surfAttrib.buffers)
       free(surfAttrib.buffers);
#else
    INFO("Use local buffer for decoding");
    for (i = 0; i < mBufCount; i++) {
        mSurfArray[i] = new VaapiSurface(display,
                                        VAAPI_CHROMA_TYPE_YUV420,
                                        config->width,
                                        config->height,
                                        NULL);
       if (!mSurfArray[i]) {
           ERROR("VaapiSurface allocation failed ");
           return;
       }
   }

   mUseExtBuf = false;
#endif

    /* initialize all surface buffers in the pool */
    for (i = 0; i < mBufCount; i++) {
         mBufArray[i] = (struct VideoSurfaceBuffer*)malloc(sizeof(struct VideoSurfaceBuffer));
         mBufArray[i]->pictureOrder             = INVALID_POC;
         mBufArray[i]->referenceFrame           = false;
         mBufArray[i]->asReferernce             = false; //todo fix the typo
         mBufArray[i]->mappedData               = NULL;
         mBufArray[i]->renderBuffer.display     = display;
         mBufArray[i]->renderBuffer.surface     = mSurfArray[i]->getID();
         mBufArray[i]->renderBuffer.scanFormat  = VA_FRAME_PICTURE;
         mBufArray[i]->renderBuffer.timeStamp   = INVALID_PTS;
         mBufArray[i]->renderBuffer.flag        = 0;
         mBufArray[i]->renderBuffer.graphicBufferIndex = i;
         if (mUseExtBuf) {
             /* buffers is hold by external client, such as graphics */
             mBufArray[i]->renderBuffer.graphicBufferHandle  = config->graphicBufferHandler[i];
             mBufArray[i]->renderBuffer.renderDone  = false;
             mBufArray[i]->status = SURFACE_RENDERING;
         } else {
             mBufArray[i]->renderBuffer.graphicBufferHandle  = NULL;
             mBufArray[i]->renderBuffer.renderDone  = true;
             mBufArray[i]->status = SURFACE_FREE;
         }
     }
   
    mBufMapped = false;
    if (config->flag & WANT_RAW_OUTPUT) {
       //mapSurfaceBuffers();
      // mBufMapped = true;
       INFO("Surface is mapped out for raw output ");
    }
}

VaapiSurfaceBufferPool::~VaapiSurfaceBufferPool()
{
    INFO(" destruct the render buffer pool ");
    uint32_t i;

    if (mBufMapped) {
        unmapSurfaceBuffers();
        mBufMapped = false;
     }
              
    for (i = 0; i < mBufCount; i++) {
         delete mSurfArray[i];
         mSurfArray[i] = NULL;
    }
    free(mSurfArray);
    mSurfArray = NULL;
   
    for (i = 0; i < mBufCount; i++) {
         free(mBufArray[i]);
         mBufArray[i] = NULL;
    }
    free(mBufArray);
    mBufArray = NULL;

    pthread_cond_destroy(&mCond);
    pthread_mutex_destroy(&mLock);
}


VideoSurfaceBuffer *
VaapiSurfaceBufferPool::acquireFreeBuffer()
{
     VideoSurfaceBuffer *surfBuf = searchAvailableBuffer();
   
     if (surfBuf) {
         pthread_mutex_lock(&mLock);
         surfBuf->renderBuffer.driverRenderDone = true; 
         surfBuf->renderBuffer.timeStamp  = INVALID_PTS;
         surfBuf->pictureOrder = INVALID_POC; 
         surfBuf->status = SURFACE_DECODING;
         pthread_mutex_unlock(&mLock);
     }
     return surfBuf;
}

VideoSurfaceBuffer *
VaapiSurfaceBufferPool::acquireFreeBufferWithWait()
{
     VideoSurfaceBuffer *surfBuf = searchAvailableBuffer();
 
     if (!surfBuf) {
         INFO("Waiting for avaiable surface buffer");
         pthread_mutex_lock(&mLock);
         pthread_cond_wait(&mCond, &mLock);
         pthread_mutex_unlock(&mLock);
     }
    
     while(!surfBuf) { 
         surfBuf = acquireFreeBuffer();
     }

     return surfBuf;
}

bool VaapiSurfaceBufferPool::setReferenceInfo(
    VideoSurfaceBuffer *buf,
    bool referenceFrame, 
    bool asReference)
{
     DEBUG("set frame reference information");
     pthread_mutex_lock(&mLock);
     buf->referenceFrame = referenceFrame; 
     buf->asReferernce   = asReference; //to fix typo error
     pthread_mutex_unlock(&mLock);
     return true;
}

bool 
VaapiSurfaceBufferPool::outputBuffer(
    VideoSurfaceBuffer *buf,
    uint64_t timeStamp, 
    uint32_t poc)
{
     DEBUG("prepare to render one surface buffer");
     uint32_t i = 0;
     pthread_mutex_lock(&mLock);
     for (i = 0; i < mBufCount; i++) {
         if (mBufArray[i] == buf) {
            mBufArray[i]->pictureOrder = poc; 
            mBufArray[i]->renderBuffer.timeStamp = timeStamp;  
            mBufArray[i]->status |= SURFACE_TO_RENDER;
            break;
          }
     }
     pthread_mutex_unlock(&mLock);
 
     if (i == mBufCount) {
        INFO("Pool: outputbuffer, Error buffer pointer ");
        return false;
     }
        
     return true;
}

bool
VaapiSurfaceBufferPool::recycleBuffer(VideoSurfaceBuffer *buf, bool is_from_render)
{
     uint32_t i;

     pthread_mutex_lock(&mLock);
     for (i = 0; i < mBufCount; i++) {
         if (mBufArray[i] == buf) {
             if (is_from_render) {
                  mBufArray[i]->renderBuffer.renderDone = true;
                  mBufArray[i]->status &= ~SURFACE_RENDERING;
                  INFO("Pool: recy from render, status = %d", mBufArray[i]->status);
             } else { //release from decoder
                  mBufArray[i]->status &= ~SURFACE_DECODING;
                  INFO("Pool: recy from decoder, status = %d", mBufArray[i]->status);
             }

             pthread_cond_signal(&mCond);
             pthread_mutex_unlock(&mLock);
             return true;
         }
     }
   
     pthread_mutex_unlock(&mLock);
     INFO("Can not find such buf in the pool ");
     return false;
}

void 
VaapiSurfaceBufferPool::flushPool()
{
     uint32_t i;

     pthread_mutex_lock(&mLock);

     for (i = 0; i < mBufCount; i++) {
         mBufArray[i]->referenceFrame = false;
         mBufArray[i]->asReferernce   = false;
         mBufArray[i]->status         = SURFACE_FREE;
     }

     pthread_mutex_unlock(&mLock);
}

void 
VaapiSurfaceBufferPool::mapSurfaceBuffers()
{
    uint32_t i;
    VideoFrameRawData *rawData;

    pthread_mutex_lock(&mLock);
    for (i = 0; i < mBufCount; i++) {
        VaapiSurface *surf = mSurfArray[i];
        VaapiImage *image  = surf->getDerivedImage();
        VaapiImageRaw *imageRaw = image->map();

        rawData = new VideoFrameRawData;
        rawData->width  = imageRaw->width; 
        rawData->height = imageRaw->height;
        rawData->fourcc = imageRaw->format;
        rawData->size   = imageRaw->size; 
        rawData->data   = imageRaw->pixels[0]; 

        for (i = 0; i < imageRaw->num_planes; i++) {
            rawData->offset[i] = imageRaw->pixels[i]
                                 - imageRaw->pixels[0];
            rawData->pitch[i]  = imageRaw->strides[i];
        }

        mBufArray[i]->renderBuffer.rawData = rawData;
        mBufArray[i]->mappedData           = rawData;
    }

    pthread_mutex_unlock(&mLock);
}

void 
VaapiSurfaceBufferPool::unmapSurfaceBuffers() 
{
    uint32_t i;
    pthread_mutex_lock(&mLock);

    for (i = 0; i < mBufCount; i++) {
         VaapiSurface *surf = mSurfArray[i];
         VaapiImage *image  = surf->getDerivedImage();
       
         image->unmap();

         delete mBufArray[i]->renderBuffer.rawData;
         mBufArray[i]->renderBuffer.rawData = NULL;
         mBufArray[i]->mappedData           = NULL;
     }

    pthread_mutex_unlock(&mLock);

} 

VideoSurfaceBuffer*
VaapiSurfaceBufferPool::getOutputByMinTimeStamp()
{
    uint32_t i;
    uint64_t pts = INVALID_PTS;     
    VideoSurfaceBuffer * buf = NULL;

    pthread_mutex_lock(&mLock);
    for (i = 0; i < mBufCount; i++) {
        if (!(mBufArray[i]->status & SURFACE_TO_RENDER))
            continue;

        if (mBufArray[i]->renderBuffer.timeStamp == INVALID_PTS ||
            mBufArray[i]->pictureOrder  == INVALID_POC)
            continue;

        if ((uint64_t)(mBufArray[i]->renderBuffer.timeStamp) < pts) {
            pts = mBufArray[i]->renderBuffer.timeStamp;
            buf = mBufArray[i]; 
        }
    }

    if (buf) {
       buf->status &= (~SURFACE_TO_RENDER);
       buf->status |= SURFACE_RENDERING;

       INFO("Pool: %x to display", buf->renderBuffer.surface);
    }// else
      // INFO("Can not found surface for display");

    pthread_mutex_unlock(&mLock);

    return buf; 
}

VaapiSurface*
VaapiSurfaceBufferPool::getVaapiSurface(VideoSurfaceBuffer *buf)
{
    uint32_t i;
   
    for (i = 0; i < mBufCount; i++) {
         if (buf->renderBuffer.surface == mSurfArray[i]->getID())
              return mSurfArray[i];
    }
     
    return NULL; 
}
    
VideoSurfaceBuffer*
VaapiSurfaceBufferPool::getBufferBySurfaceID(VASurfaceID id)
{
    uint32_t i;
   
    for (i = 0; i < mBufCount; i++) {
         if (mBufArray[i]->renderBuffer.surface == id)
              return mBufArray[i];
    }
     
    return NULL; 
}

VideoSurfaceBuffer *
VaapiSurfaceBufferPool::getBufferByHandler(void *graphicHandler)
{
    uint32_t i;

    if (!mUseExtBuf)
      return NULL;
   
    for (i = 0; i < mBufCount; i++) {
         if (mBufArray[i]->renderBuffer.graphicBufferHandle == graphicHandler)
              return mBufArray[i];
    }
     
    return NULL; 
}
     
VideoSurfaceBuffer *
VaapiSurfaceBufferPool::getBufferByIndex(uint32_t index)
{
    return mBufArray[index];
}

VideoSurfaceBuffer* 
VaapiSurfaceBufferPool::searchAvailableBuffer()
{ 
    uint32_t i;
    VAStatus vaStatus;
    VASurfaceStatus surfStatus;
    VideoSurfaceBuffer *surfBuf = NULL;

    pthread_mutex_lock(&mLock);

    for (i = 0; i < mBufCount; i++) {
         surfBuf = mBufArray[i];
         /* skip non free surface  */
         if (surfBuf->status != SURFACE_FREE)
            continue;
             
         if (!mUseExtBuf) 
             break;
             
         vaStatus = vaQuerySurfaceStatus(mDisplay,
                       surfBuf->renderBuffer.surface,
                       &surfStatus);

         if ((vaStatus == VA_STATUS_SUCCESS && 
              surfStatus == VASurfaceReady)) 
              break;
     }
     
     pthread_mutex_unlock(&mLock);
   
     if (i != mBufCount)
        return mBufArray[i];
     else
        INFO("Can not found free buffer!!!!!!1");
 
     return NULL;
 }
