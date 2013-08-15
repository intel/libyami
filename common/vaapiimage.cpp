/*
 *  vaapiimage.cpp - VA image abstraction
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

#include <stdlib.h>
#include "log.h"
#include "vaapiutils.h"
#include "vaapiimage.h"

const VAImageFormat*
VaapiImage::getVaFormat(VaapiImageFormat format)
{
    const VaapiImageFormatMap *map = NULL;
    for (map = vaapi_image_formats; map->format; map ++) {
        if (map->format == format)
            return &map->vaFormat;
    } 
    return NULL;
}

VaapiImage::VaapiImage(VADisplay display,
                       VaapiImageFormat format,
                       uint32_t width,
                       uint32_t height)
{
    VAStatus status;
    VAImageFormat *vaFormat;

    mDisplay = display;
    mFormat  = format;
    mWidth   = width;
    mHeight  = height;
    mIsMapped = false;

    vaFormat = (VAImageFormat*)getVaFormat(format);      
    if (!vaFormat) {
        ERROR("Create image failed, not supported fourcc");
        return;
    }

    status = vaCreateImage(mDisplay,
                           vaFormat,
                           mWidth,
                           mHeight,
                           &mImage);
    
    if (status != VA_STATUS_SUCCESS ||
        mImage.format.fourcc != vaFormat->fourcc) {
        ERROR("Create image failed");
        return;
    }
}

VaapiImage::VaapiImage(VADisplay display,
                       VAImage* image)
{
    VAStatus status;

    mDisplay  = display;
    mWidth    = image->width;
    mHeight   = image->height;
    mID       = image->image_id;
    mIsMapped = false;

    memcpy((void*)&mImage, (void*)image, sizeof(VAImage));
}


VaapiImage::~VaapiImage() 
{
    VAStatus status;
   
    if (mIsMapped){
       unmap();
       mIsMapped = false;
    }

    status = vaDestroyImage(mDisplay, mImage.image_id);     

    if (!vaapi_check_status(status, "vaDestoryImage()"))
        return;
}

VaapiImageRaw* VaapiImage::map() 
{
    uint32_t i;
    void  *data;
    VAStatus status;
   
    if (mIsMapped) {
        return &mRawImage;
    }

    status = vaMapBuffer(mDisplay,
                         mImage.buf,
                         &data);
   
    if (!vaapi_check_status(status, "vaMapBuffer()"))
        return NULL;
   
     mRawImage.format     = mFormat;
     mRawImage.width      = mWidth;
     mRawImage.height     = mHeight;
     mRawImage.num_planes = mImage.num_planes;
     mRawImage.size       = mImage.data_size;

     for (i = 0; i < mImage.num_planes; i ++) {
         mRawImage.pixels[i] = (uint8_t *)((uint32_t)data + mImage.offsets[i]);
         mRawImage.strides[i] = mImage.pitches[i];
     }
     mIsMapped = true;
   
     return &mRawImage;
}

bool VaapiImage::unmap()
{
    VAStatus status;
 
    if (!mIsMapped)
       return true;
    
    status = vaUnmapBuffer(mDisplay,
                           mImage.buf);
    if (!vaapi_check_status(status, "vaUnmapBuffer()"))
        return false;

    mIsMapped  = false;

    return true;
}

bool VaapiImage::isMapped()
{
    return mIsMapped;
}

VaapiImageFormat VaapiImage::getFormat()
{
   return mFormat;
}

VAImageID VaapiImage::getID()
{
   return mID;
}

uint32_t VaapiImage::getWidth()
{
    return mWidth;
}

uint32_t VaapiImage::getHeight()
{
   return mHeight;
}


