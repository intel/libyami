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

#include <assert.h>
#include "vaapipicture.h"
#include "common/vaapiutils.h"

/* picture class implementation */
VaapiPicture::VaapiPicture(VADisplay display,
                           VAContextID context,
                           VaapiSurfaceBufferPool *surfBufPool,
                           VaapiPictureStructure structure)
:mDisplay(display), mContext(context), 
 mSurfBufPool(surfBufPool), mStructure(structure)
{
    mPicParam  = NULL;
    mIqMatrix  = NULL;
    mBitPlane  = NULL;
    mHufTable  = NULL;

    mTimeStamp = INVALID_PTS;
    mPoc       = INVALID_POC;
    mFlags     = 0;
    mSurfBuf   = NULL;
    mSurfaceID = 0;

    if (mSurfBufPool) {
        mSurfBuf = mSurfBufPool->acquireFreeBuffer();
        if (mSurfBuf) {
            mSurfaceID = mSurfBuf->renderBuffer.surface;
        }
        else
        ERROR("VaapiPicture: acquire surface fail");
    }
}

VaapiPicture::~VaapiPicture()
{
   vector<VaapiSlice*>::iterator iter;

    if(mPicParam){
       delete mPicParam;
       mPicParam  = NULL;
    }
 
    if(mIqMatrix){
       delete mIqMatrix;
       mIqMatrix  = NULL;
    }
  
    if(mBitPlane){
       delete mBitPlane;
       mBitPlane  = NULL;
    }

    if (mHufTable) {
       delete mHufTable;
       mHufTable  = NULL;
    }

    for (iter = mSliceArray.begin(); 
          iter != mSliceArray.end(); iter++)
        delete *iter;
    //mSliceArray.clear();

   if (mSurfBufPool && mSurfBuf){
      mSurfBufPool->recycleBuffer(mSurfBuf, false);
      mSurfBuf = NULL;
   }

} 

void VaapiPicture::addSlice(VaapiSlice *slice)
{
   mSliceArray.push_back(slice);
}

VaapiSlice* VaapiPicture::getLastSlice()
{
   return mSliceArray.back();
}

bool VaapiPicture::decodePicture()
{
    VAStatus status;
    uint32_t i;
    vector<VaapiSlice*>::iterator iter;
    VABufferID bufferID;

    DEBUG("VP: decode picture 0x%08x", mSurfaceID);

    status = vaBeginPicture(mDisplay, mContext, mSurfaceID);
    if (!vaapi_check_status(status, "vaBeginPicture()"))
        return false;

    if (mPicParam) {
        bufferID = mPicParam->getID();
        status = vaRenderPicture(mDisplay, mContext, &bufferID, 1);
        if (!vaapi_check_status(status, "vaRenderPicture(), render pic param"))
           return false;
    }

    if (mIqMatrix) {
        bufferID = mIqMatrix->getID();
        status = vaRenderPicture(mDisplay, mContext, &bufferID, 1);
        if (!vaapi_check_status(status, "vaRenderPicture(), render IQ matrix"))
            return false;
    }

    if (mBitPlane) {
        bufferID = mBitPlane->getID();
        status = vaRenderPicture(mDisplay, mContext, &bufferID, 1);
        if (!vaapi_check_status(status, "vaRenderPicture(), render bit plane"))
            return false;
    }

    if (mHufTable) {
        bufferID = mHufTable->getID();
        status = vaRenderPicture(mDisplay, mContext, &bufferID, 1);
        if (!vaapi_check_status(status, "vaRenderPicture(), render huf table"))
            return false;
    }

    for (iter = mSliceArray.begin(); iter != mSliceArray.end(); iter++) {
        VaapiBufObject *paramBuf = (*iter)->mParam;
        VaapiBufObject *dataBuf  = (*iter)->mData;
        VABufferID vaBuffers[2];

        vaBuffers[0] = paramBuf->getID();
        vaBuffers[1] = dataBuf->getID();

        status = vaRenderPicture(mDisplay, mContext, vaBuffers, 2);
        if (!vaapi_check_status(status, "vaRenderPicture()"))
            return false;
    }

    status = vaEndPicture(mDisplay, mContext);
    if (!vaapi_check_status(status, "vaEndPicture()"))
        return false;
    return true;
}

bool VaapiPicture::output()
{
    bool isReferenceFrame = false;
    bool usedAsReference = false;

    if (!mSurfBufPool || !mSurfBuf) {
       ERROR("no surface buffer pool ");
       return false;
    }

    if (mType == VAAPI_PICTURE_TYPE_I ||
        mType == VAAPI_PICTURE_TYPE_P) 
        isReferenceFrame = true;

    if(mFlags & VAAPI_PICTURE_FLAG_REFERENCE)
         usedAsReference = true;
  
    mSurfBufPool->setReferenceInfo(mSurfBuf, isReferenceFrame, usedAsReference);

    return mSurfBufPool->outputBuffer(mSurfBuf, mTimeStamp, mPoc);
}


