/*
 *  vaapisurfacepool.c - VA surface pool
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
#include "log.h"
#include "vaapisurface_pool.h"

VaapiSurfacePool::VaapiSurfacePool(VADisplay display,
    VaapiChromaType chromaType,
    uint32_t  width,
    uint32_t  height,
    uint32_t  count,
    VASurfaceAttributeTPI *surfAttrib)
:mSurfCount(count)
{
    uint32_t i;
    mSurfArray =  (VaapiSurface**) malloc (sizeof(VaapiSurface*) * mSurfCount);
    mUsedFlagArray = (bool*) malloc (sizeof(bool)*mSurfCount);

    if (surfAttrib && surfAttrib->count < count) {
          ERROR(" surface handle is not enough for the pool ");
          return;
     }
  
    for (i = 0; i < mSurfCount ; i++) {
        if (!surfAttrib) {
             mSurfArray[i] = new VaapiSurface(display, 
                                              chromaType,
                                              width, 
                                              height,
                                              NULL);
         } else {
              surfAttrib->buffers[0] =  surfAttrib->buffers[i];
              mSurfArray[i] = new VaapiSurface(display, 
                                              chromaType,
                                              width, 
                                              height,
                                              surfAttrib);
        }

       if (!mSurfArray[i]) {
           ERROR("VaapiSurfacePool allocation failed ");
           return;
       }
       mUsedFlagArray[i] = false;
    }

    mSurfUsed = 0;
    pthread_cond_init(&mCond, NULL);
    pthread_mutex_init(&mLock, NULL);
}

VaapiSurfacePool::~VaapiSurfacePool()
{
    uint32_t i;
 
    for (i = 0; i < mSurfCount ; i++) {
        delete mSurfArray[i];
    }

    free(mSurfArray);
    free(mUsedFlagArray);

    pthread_cond_destroy(&mCond);
    pthread_mutex_destroy(&mLock);
}

VaapiSurface *
VaapiSurfacePool::acquireSurface()
{
    uint32_t i;
    VaapiSurface *surf = NULL;

    pthread_mutex_lock(&mLock);

    if (mSurfUsed == mSurfCount){
        WARNING("Not avaliable surface now ");
        pthread_mutex_unlock(&mLock);
        return NULL;
    }
   
    for (i = 0; i < mSurfCount; i++) {
        if (mUsedFlagArray[i] == false)
            break;
    }
       
    if (i == mSurfCount) {     
        WARNING("Not avaliable surface now ");
        pthread_mutex_unlock(&mLock);
        return NULL;
    }

    surf = mSurfArray[i]; 
    mUsedFlagArray[i] = true;
    mSurfUsed ++;

    pthread_mutex_unlock(&mLock);

    return surf; 
}

VaapiSurface *
VaapiSurfacePool::acquireSurfaceWithWait()
{
    uint32_t i;
    uint32_t mCurrentPos;   
    VaapiSurface *surf = NULL;
    
    pthread_mutex_lock(&mLock);

    if (mSurfUsed == mSurfCount){
        WARNING("No avaliable surface now, wait here ");
        pthread_cond_wait(&mCond, &mLock);
   }

    for (i = 0; i < mSurfCount; i++) {
        if (mUsedFlagArray[i] == false)
            break;
    }
       
    if (i == mSurfCount) {     
        WARNING("Should not happen here ");
        pthread_mutex_unlock(&mLock);
        return NULL;
    }

    surf = mSurfArray[i]; 
    mUsedFlagArray[i] = true;
    mSurfUsed ++;

    pthread_mutex_unlock(&mLock);

    return surf; 
}

bool 
VaapiSurfacePool::releaseSurface(VaapiSurface *surf)
{
    uint32_t i;
    uint32_t mCurrentPos;   
    
    pthread_mutex_lock(&mLock);

    for (i = 0; i < mSurfCount; i++) {
        if (surf->getID() == mSurfArray[i]->getID())
          break;
    }
 
    if (i == mSurfCount) {     
        WARNING("this surf does not below to this pool ");
        pthread_mutex_unlock(&mLock);
        return false;
    }

    mUsedFlagArray[i] = false;
    mSurfUsed--;

    pthread_cond_signal(&mCond);
    pthread_mutex_unlock(&mLock);
  
    return true;
}
 
