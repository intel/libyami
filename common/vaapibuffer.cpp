/*
 *  vaapicodecobject.cpp - Basic object used for decode/encode
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

#include <va/va.h>
#include "log.h"
#include "vaapiutils.h"
#include "vaapibuffer.h"

VaapiBufObject::VaapiBufObject(
    VADisplay   display,
    VAContextID context, 
    uint32_t  bufType,
    void      *data,
    uint32_t  size)
:mDisplay(display), mSize(size), mBuf(NULL)
{
    VAStatus status;

    if (size == 0) {
       ERROR("buffer size is zero");
       return ;    
    }

    if(!vaapi_create_buffer(display, 
           context,
           bufType,
           size,
           data,
           &mBufID,
           (void**)0)){
       ERROR("create buffer failed"); 
       return ;    
    }

}

VaapiBufObject::~VaapiBufObject()
{
    if (mBuf) {
        vaapi_unmap_buffer(mDisplay, mBufID, &mBuf);
        mBuf = NULL;
    }
      
    vaapi_destroy_buffer(mDisplay, &mBufID);
}
 
VABufferID VaapiBufObject::getID()
{
    return mBufID;
}

uint32_t VaapiBufObject::getSize()
{
   return mSize;
}

void* VaapiBufObject::map()
{
   if (mBuf)
      return mBuf;

   mBuf = vaapi_map_buffer(mDisplay, mBufID);
   return mBuf;
}

void VaapiBufObject::unmap()
{
    if (mBuf) {
        vaapi_unmap_buffer(mDisplay, mBufID, &mBuf);
        mBuf = NULL;
    }
}

