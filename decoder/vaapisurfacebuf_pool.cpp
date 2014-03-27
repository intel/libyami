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

VaapiSurfaceBufferPool::VaapiSurfaceBufferPool(VADisplay display,
					       VideoConfigBuffer * config)
:mDisplay(display)
{
    INFO("Construct the render buffer pool ");

    uint32_t i;
    uint32_t format = VA_RT_FORMAT_YUV420;

    pthread_cond_init(&mCond, NULL);
    pthread_mutex_init(&mLock, NULL);

    if (!config) {
	ERROR("Wrong parameter to init render buffer pool");
	return;
    }

    mBufCount = config->surfaceNumber;
    mBufArray =
	(VideoSurfaceBuffer **) malloc(sizeof(VideoSurfaceBuffer *) *
				       mBufCount);
    mSurfArray =
	(VaapiSurface **) malloc(sizeof(VaapiSurface *) * mBufCount);

    if (config->flag & WANT_SURFACE_PROTECTION) {
	format != VA_RT_FORMAT_PROTECTED;
	INFO("Surface is protectd ");
    }

    /* allocate surface for the pool */
    mUseExtBuf = false;
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

	for (i = 0; i < mBufCount; i++) {
	    surfAttribExtBuf.buffers[0] =
		(unsigned long) config->graphicBufferHandler[i];
	    mSurfArray[i] =
		new VaapiSurface(display, VAAPI_CHROMA_TYPE_YUV420,
				 config->graphicBufferWidth,
				 config->graphicBufferHeight,
				 (void *) &surfaceAttribs, 2);

	    if (!mSurfArray[i]) {
		ERROR("VaapiSurface allocation failed ");
		return;
	    }
	}

	mUseExtBuf = true;

	if (surfAttribExtBuf.buffers)
	    free(surfAttribExtBuf.buffers);

    } else {
	for (i = 0; i < mBufCount; i++) {
	    mSurfArray[i] = new VaapiSurface(display,
					     VAAPI_CHROMA_TYPE_YUV420,
					     config->width,
					     config->height, NULL, 0);
	    if (!mSurfArray[i]) {
		ERROR("VaapiSurface allocation failed ");
		return;
	    }
	}
    }

    /* Wrap vaapi surfaces into VideoSurfaceBuffer pool */
    for (i = 0; i < mBufCount; i++) {
	mBufArray[i] =
	    (struct VideoSurfaceBuffer *)
	    malloc(sizeof(struct VideoSurfaceBuffer));
	mBufArray[i]->pictureOrder = INVALID_POC;
	mBufArray[i]->referenceFrame = false;
	mBufArray[i]->asReferernce = false;	//todo fix the typo
	mBufArray[i]->mappedData = NULL;
	mBufArray[i]->renderBuffer.display = display;
	mBufArray[i]->renderBuffer.surface = mSurfArray[i]->getID();
	mBufArray[i]->renderBuffer.scanFormat = VA_FRAME_PICTURE;
	mBufArray[i]->renderBuffer.timeStamp = INVALID_PTS;
	mBufArray[i]->renderBuffer.flag = 0;
	mBufArray[i]->renderBuffer.graphicBufferIndex = i;
	if (mUseExtBuf) {
	    /* buffers is hold by external client, such as graphics */
	    mBufArray[i]->renderBuffer.graphicBufferHandle =
		config->graphicBufferHandler[i];
	    mBufArray[i]->renderBuffer.renderDone = false;
	    mBufArray[i]->status = SURFACE_RENDERING;
	} else {
	    mBufArray[i]->renderBuffer.graphicBufferHandle = NULL;
	    mBufArray[i]->renderBuffer.renderDone = true;
	    mBufArray[i]->status = SURFACE_FREE;
	}
    }

    mBufMapped = false;
    if (config->flag & WANT_RAW_OUTPUT) {
	mapSurfaceBuffers();
	mBufMapped = true;
	INFO("Surface is mapped out for raw output ");
    }
}

VaapiSurfaceBufferPool::~VaapiSurfaceBufferPool()
{
    INFO("Destruct the render buffer pool ");
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


VideoSurfaceBuffer *VaapiSurfaceBufferPool::acquireFreeBuffer()
{
    VideoSurfaceBuffer *surfBuf = searchAvailableBuffer();

    if (surfBuf) {
	pthread_mutex_lock(&mLock);
	surfBuf->renderBuffer.driverRenderDone = true;
	surfBuf->renderBuffer.timeStamp = INVALID_PTS;
	surfBuf->pictureOrder = INVALID_POC;
	surfBuf->status = SURFACE_DECODING;
	pthread_mutex_unlock(&mLock);
    }
    return surfBuf;
}

VideoSurfaceBuffer *VaapiSurfaceBufferPool::acquireFreeBufferWithWait()
{
    VideoSurfaceBuffer *surfBuf = searchAvailableBuffer();

    if (!surfBuf) {
	INFO("Waiting for free surface buffer");
	pthread_mutex_lock(&mLock);
	pthread_cond_wait(&mCond, &mLock);
	pthread_mutex_unlock(&mLock);
	INFO("Receive signal about a free buffer availble");
	surfBuf = searchAvailableBuffer();
	if (!surfBuf) {
	    ERROR
		("Can not get the released surface, this should not happen");
	}
    }

    if (surfBuf) {
	pthread_mutex_lock(&mLock);
	surfBuf->renderBuffer.driverRenderDone = true;
	surfBuf->renderBuffer.timeStamp = INVALID_PTS;
	surfBuf->pictureOrder = INVALID_POC;
	surfBuf->status = SURFACE_DECODING;
	pthread_mutex_unlock(&mLock);
    }

    return surfBuf;
}

bool VaapiSurfaceBufferPool::setReferenceInfo(VideoSurfaceBuffer * buf,
					      bool referenceFrame,
					      bool asReference)
{
    pthread_mutex_lock(&mLock);
    buf->referenceFrame = referenceFrame;
    buf->asReferernce = asReference;	//to fix typo error
    pthread_mutex_unlock(&mLock);
    return true;
}

bool VaapiSurfaceBufferPool::outputBuffer(VideoSurfaceBuffer * buf,
					  uint64_t timeStamp, uint32_t poc)
{
    DEBUG("Pool: set surface(ID:%8x, poc:%x) to be rendered",
	  buf->renderBuffer.surface, poc);

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
	ERROR("Pool: outputbuffer, Error buffer pointer ");
	return false;
    }

    return true;
}

bool VaapiSurfaceBufferPool::recycleBuffer(VideoSurfaceBuffer * buf,
					   bool is_from_render)
{
    uint32_t i;

    pthread_mutex_lock(&mLock);
    for (i = 0; i < mBufCount; i++) {
	if (mBufArray[i] == buf) {
	    if (is_from_render) {	//release from decoder
		mBufArray[i]->renderBuffer.renderDone = true;
		mBufArray[i]->status &= ~SURFACE_RENDERING;
		DEBUG("Pool: recy surf(%8x) from render, status = %d",
		      buf->renderBuffer.surface, mBufArray[i]->status);
	    } else {		//release from decoder
		mBufArray[i]->status &= ~SURFACE_DECODING;
		DEBUG("Pool: recy surf(%8x) from decoder, status = %d",
		      buf->renderBuffer.surface, mBufArray[i]->status);
	    }

	    if (mBufArray[i]->status == SURFACE_FREE) {
		pthread_cond_signal(&mCond);
	    }

	    pthread_mutex_unlock(&mLock);
	    return true;
	}
    }

    pthread_mutex_unlock(&mLock);

    ERROR("recycleBuffer: Can not find buf=%p in the pool", buf);
    return false;
}

void VaapiSurfaceBufferPool::flushPool()
{
    uint32_t i;

    pthread_mutex_lock(&mLock);

    for (i = 0; i < mBufCount; i++) {
	mBufArray[i]->referenceFrame = false;
	mBufArray[i]->asReferernce = false;
	mBufArray[i]->status = SURFACE_FREE;
    }

    pthread_mutex_unlock(&mLock);
}

void VaapiSurfaceBufferPool::mapSurfaceBuffers()
{
    uint32_t i;
    VideoFrameRawData *rawData;

    pthread_mutex_lock(&mLock);
    for (i = 0; i < mBufCount; i++) {
	VaapiSurface *surf = mSurfArray[i];
	VaapiImage *image = surf->getDerivedImage();
	VaapiImageRaw *imageRaw = image->map();

	rawData = new VideoFrameRawData;
	rawData->width = imageRaw->width;
	rawData->height = imageRaw->height;
	rawData->fourcc = imageRaw->format;
	rawData->size = imageRaw->size;
	rawData->data = imageRaw->pixels[0];

	for (i = 0; i < imageRaw->num_planes; i++) {
	    rawData->offset[i] = imageRaw->pixels[i]
		- imageRaw->pixels[0];
	    rawData->pitch[i] = imageRaw->strides[i];
	}

	mBufArray[i]->renderBuffer.rawData = rawData;
	mBufArray[i]->mappedData = rawData;
    }

    pthread_mutex_unlock(&mLock);
}

void VaapiSurfaceBufferPool::unmapSurfaceBuffers()
{
    uint32_t i;
    pthread_mutex_lock(&mLock);

    for (i = 0; i < mBufCount; i++) {
	VaapiSurface *surf = mSurfArray[i];
	VaapiImage *image = surf->getDerivedImage();

	image->unmap();

	delete mBufArray[i]->renderBuffer.rawData;
	mBufArray[i]->renderBuffer.rawData = NULL;
	mBufArray[i]->mappedData = NULL;
    }

    pthread_mutex_unlock(&mLock);

}

VideoSurfaceBuffer *VaapiSurfaceBufferPool::getOutputByMinTimeStamp()
{
    uint32_t i;
    uint64_t pts = INVALID_PTS;
    VideoSurfaceBuffer *buf = NULL;

    pthread_mutex_lock(&mLock);
    for (i = 0; i < mBufCount; i++) {
	if (!(mBufArray[i]->status & SURFACE_TO_RENDER))
	    continue;

	if (mBufArray[i]->renderBuffer.timeStamp == INVALID_PTS)
	    continue;

	if ((uint64_t) (mBufArray[i]->renderBuffer.timeStamp) < pts) {
	    pts = mBufArray[i]->renderBuffer.timeStamp;
	    buf = mBufArray[i];
	}
    }

    if (buf) {
	buf->status &= (~SURFACE_TO_RENDER);
	buf->status |= SURFACE_RENDERING;

	DEBUG("Pool: found %x to display with MIN pts = %d",
	      buf->renderBuffer.surface, pts);
    }

    pthread_mutex_unlock(&mLock);

    return buf;
}

VideoSurfaceBuffer *VaapiSurfaceBufferPool::getOutputByMinPOC()
{
    uint32_t i;
    uint32_t poc = INVALID_POC;
    VideoSurfaceBuffer *buf = NULL;

    pthread_mutex_lock(&mLock);
    for (i = 0; i < mBufCount; i++) {
	if (!(mBufArray[i]->status & SURFACE_TO_RENDER))
	    continue;

	if (mBufArray[i]->renderBuffer.timeStamp == INVALID_PTS ||
	    mBufArray[i]->pictureOrder == INVALID_POC)
	    continue;

	if ((uint64_t) (mBufArray[i]->pictureOrder) < poc) {
	    poc = mBufArray[i]->pictureOrder;
	    buf = mBufArray[i];
	}
    }

    if (buf) {
	buf->status &= (~SURFACE_TO_RENDER);
	buf->status |= SURFACE_RENDERING;

	DEBUG("Pool: found %x to display with MIN poc = %x",
	      buf->renderBuffer.surface, poc);
    }

    pthread_mutex_unlock(&mLock);

    return buf;
}

VaapiSurface *VaapiSurfaceBufferPool::getVaapiSurface(VideoSurfaceBuffer *
						      buf)
{
    uint32_t i;

    for (i = 0; i < mBufCount; i++) {
	if (buf->renderBuffer.surface == mSurfArray[i]->getID())
	    return mSurfArray[i];
    }

    return NULL;
}

VideoSurfaceBuffer *VaapiSurfaceBufferPool::
getBufferBySurfaceID(VASurfaceID id)
{
    uint32_t i;

    for (i = 0; i < mBufCount; i++) {
	if (mBufArray[i]->renderBuffer.surface == id)
	    return mBufArray[i];
    }

    return NULL;
}

VideoSurfaceBuffer *VaapiSurfaceBufferPool::
getBufferByHandler(void *graphicHandler)
{
    uint32_t i;

    if (!mUseExtBuf)
	return NULL;

    for (i = 0; i < mBufCount; i++) {
	if (mBufArray[i]->renderBuffer.graphicBufferHandle ==
	    graphicHandler)
	    return mBufArray[i];
    }

    return NULL;
}

VideoSurfaceBuffer *VaapiSurfaceBufferPool::
getBufferByIndex(uint32_t index)
{
    return mBufArray[index];
}

VideoSurfaceBuffer *VaapiSurfaceBufferPool::searchAvailableBuffer()
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
	WARNING("Can not found availabe buffer");

    return NULL;
}
