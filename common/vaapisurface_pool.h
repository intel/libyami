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

#ifndef VAAPI_SURFACE_POOL_H
#define VAAPI_SURFACE_POOL_H

#include <pthread.h>
#include <semaphore.h>
#include <va/va_tpi.h>
#include "vaapitypes.h"
#include "vaapisurface.h"

class VaapiSurfacePool {
public: 
   VaapiSurfacePool(VADisplay display,
                 VaapiChromaType chromaType,
                 uint32_t  width,
                 uint32_t  height,
                 uint32_t  count,
                 VASurfaceAttributeTPI *surfAttrib);

   ~VaapiSurfacePool();

   VaapiSurface *acquireSurface();
   VaapiSurface *acquireSurfaceWithWait();
   bool releaseSurface(VaapiSurface *surf);   

private:
   VaapiSurface **mSurfArray;
   bool    *mUsedFlagArray;
   uint32_t mSurfCount;   
   uint32_t mSurfUsed;
   pthread_mutex_t mLock;
   pthread_cond_t  mCond;
}; 

#endif
 
