/*
 *  vaapisurface.cpp - VA surface abstraction
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

#include "log.h"
#include "vaapiutils.h"
#include "vaapisurface.h"

VaapiSurface::VaapiSurface(VADisplay display,
                 VaapiChromaType chromaType,
                 uint32_t  width,
                 uint32_t  height,
                 VASurfaceAttributeTPI *surfAttrib)
                 : mDisplay(display), mChromaType(chromaType), 
                   mWidth(width), mHeight(height)
{
    VAStatus status;
    uint32_t format;

    switch (mChromaType) {
      case VAAPI_CHROMA_TYPE_YUV420:
          format = VA_RT_FORMAT_YUV420;
          break;
      case VAAPI_CHROMA_TYPE_YUV422:
          format = VA_RT_FORMAT_YUV422;
          break;
      case VAAPI_CHROMA_TYPE_YUV444:
          format = VA_RT_FORMAT_YUV444;
          break;
      default:
          format = VA_RT_FORMAT_YUV420;
          break;
    }

   if (surfAttrib) {
       status = vaCreateSurfacesWithAttribute(
                   mDisplay,
                   width,
                   height,
                   format,
                   1,
                   &mID,
                   surfAttrib);

        if (!vaapi_check_status(status, "vaCreateSurfacesWithAttribute()"))
            return;

        mExternalBufHandle = surfAttrib->buffers[0];
    } else {
        status = vaCreateSurfaces(
            mDisplay,
            format,
            mWidth,
            mHeight,
            &mID,
            1,
            NULL,
            0);

        if (!vaapi_check_status(status, "vaCreateSurfaces()"))
            return;
        
        mExternalBufHandle = 0;
    }

    mDerivedImage = NULL;
}

VaapiSurface::~VaapiSurface()
{
    VAStatus status;

    if (mDerivedImage) {
        delete mDerivedImage;
    }

    status = vaDestroySurfaces(
             mDisplay,
             &mID,
             1);
            
    if (!vaapi_check_status(status, "vaDestroySurfaces()"))
         WARNING("failed to destroy surface");  
}


VaapiChromaType VaapiSurface::getChromaType(void)
{
    return mChromaType;
}

VASurfaceID VaapiSurface::getID(void)
{
    return mID;
}

uint32_t VaapiSurface::getWidth(void)
{
   return mWidth;
}

uint32_t VaapiSurface::getHeight(void)
{
   return mHeight;
}

uint32_t VaapiSurface::getExtBufHandle(void)
{
   return mExternalBufHandle;
}

bool VaapiSurface::getImage(VaapiImage *image)
{
    VAImageID imageID;
    VAStatus status;
    uint32_t width, height;

    if (!image) return false;

    width  = image->getWidth();
    height = image->getHeight();
     
    if (width != mWidth || height != mHeight) {
        ERROR("resolution not matched \n");
        return false;
    }
    
    imageID = image->getID();
     
    status = vaGetImage(
        mDisplay,
        mID,
        0, 0, width, height,
        imageID
    );

    if (!vaapi_check_status(status, "vaGetImage()"))
        return false;

    return true;
}

bool VaapiSurface::putImage(VaapiImage *image)
{
    VAImageID imageID;
    VAStatus status;
    uint32_t width, height;

    if (!image) return false;

    width  = image->getWidth();
    height = image->getHeight();
     
    if (width != mWidth || height != mHeight) {
        ERROR("Image resolution does not match with surface");
        return false;
    }
    
    imageID = image->getID();
 
    status = vaGetImage(
        mDisplay,
        mID,
        0, 0, width, height,
        imageID
    );
 
    if (!vaapi_check_status(status, "vaPutImage()"))
        return false;

    return true;
}

VaapiImage * VaapiSurface::getDerivedImage()
{
    VAImage va_image;
    VAStatus status;

    if (mDerivedImage)
       return mDerivedImage;
   
    va_image.image_id = VA_INVALID_ID;
    va_image.buf      = VA_INVALID_ID;

    status = vaDeriveImage(
        mDisplay,
        mID,
        &va_image
    );
    if (!vaapi_check_status(status, "vaDeriveImage()"))
        return NULL;

    mDerivedImage = new VaapiImage(mDisplay, &va_image);

    return mDerivedImage;
}

bool VaapiSurface::sync()
{
    VAStatus status;

    status = vaSyncSurface(
        (VADisplay) mDisplay,
        (VASurfaceID) mID
    );

    if (!vaapi_check_status(status, "vaSyncSurface()"))
        return false;

    return true;
}

bool VaapiSurface::queryStatus(VaapiSurfaceStatus *pStatus)
{
    VASurfaceStatus surfaceStatus;
    VAStatus status;

    if (!pStatus)
        return false;

    status = vaQuerySurfaceStatus(
        (VADisplay) mDisplay,
        (VASurfaceID) mID,
        &surfaceStatus
    );

    if (!vaapi_check_status(status, "vaQuerySurfaceStatus()"))
        return false;

    *pStatus =(VaapiSurfaceStatus)toVaapiSurfaceStatus(surfaceStatus);
    return true;
}

uint32_t VaapiSurface::toVaapiSurfaceStatus(uint32_t vaFlags)
{
    uint32_t flags;
    const uint32_t vaFlagMask = (VASurfaceReady      |
                                 VASurfaceRendering  |
                                 VASurfaceDisplaying |
                                 VASurfaceSkipped);

    switch (vaFlags & vaFlagMask) {
    case VASurfaceReady:
        flags = VAAPI_SURFACE_STATUS_IDLE;
        break;
    case VASurfaceRendering:
        flags = VAAPI_SURFACE_STATUS_RENDERING;
        break;
    case VASurfaceDisplaying:
        flags = VAAPI_SURFACE_STATUS_DISPLAYING;
        break;
    case VASurfaceSkipped:
        flags = VAAPI_SURFACE_STATUS_SKIPPED;
        break; 
    default:
        flags = 0;
        break;
    }

    return flags;
}   
