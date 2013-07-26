/*
 *  vaapisurfacepool.h - VA surface pool
 *
 *  Copyright (C) 2010-2011 Splitted-Desktop Systems
 *  Copyright (C) 2011 Intel Corporation
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

#ifndef VAAPI_SURFACE_BUFFER_POOL_H
#define VAAPI_SURFACE_BUFFER_POOL_H

#include <pthread.h>
#include <semaphore.h>
#include "common/vaapisurface.h"
#include "interface/VideoDecoderDefs.h"

#define INVALID_PTS ((uint64_t)-1)
#define INVALID_POC ((uint32_t)-1)
#define MAXIMUM_POC  0x7FFFFFFF
#define MINIMUM_POC  0x80000000

enum {
   QUERY_BY_INDEX = 0,
   QUERY_BY_SURFACE_ID,
   QUERY_BY_HANDLE
};

class VaapiSurfaceBufferPool {
public: 
   VaapiSurfaceBufferPool(VADisplay display,
                          VideoConfigBuffer* config);
   ~VaapiSurfaceBufferPool();

   VideoSurfaceBuffer* searchAvailableBuffer();
   VideoSurfaceBuffer* acquireFreeBuffer();
   VideoSurfaceBuffer* acquireFreeBufferWithWait();
   bool                recycleBuffer(VideoSurfaceBuffer *buf, bool is_from_render);
   void                flushPool();

   VideoSurfaceBuffer* getOutputByMinTimeStamp();
   VideoSurfaceBuffer* getBufferByHandler(void *graphicHandle);
   VideoSurfaceBuffer* getBufferBySurfaceID(VASurfaceID id);
   VideoSurfaceBuffer* getBufferByIndex(uint32_t index);
   VaapiSurface *      getVaapiSurface(VideoSurfaceBuffer *buf);
   bool                outputBuffer(VideoSurfaceBuffer *buf, 
                                    uint64_t timeStamp, 
                                    uint32_t poc);
   bool                setReferenceInfo(VideoSurfaceBuffer *buf,
                                    bool referenceFrame,
                                    bool asReference);

private:
   void  mapSurfaceBuffers();
   void  unmapSurfaceBuffers();

private:
   VADisplay mDisplay;
   VideoSurfaceBuffer **mBufArray;
   VaapiSurface **mSurfArray;

   uint32_t mBufCount;   
   uint32_t mFreeCount;   

   bool mUseExtBuf;
   bool mBufMapped;

   pthread_mutex_t mLock;
   pthread_cond_t  mCond;
};

#endif 
