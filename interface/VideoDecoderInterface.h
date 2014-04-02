/*
 *  VideoDecoderInterface.h- basic va decoder for video
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


#ifndef VIDEO_DECODER_INTERFACE_H_
#define VIDEO_DECODER_INTERFACE_H_

#include "VideoDecoderDefs.h"
#include <X11/Xlib.h>

class IVideoDecoder {
  public:
    virtual ~ IVideoDecoder() {
    }
    virtual Decode_Status start(VideoConfigBuffer * buffer) = 0;
    virtual Decode_Status reset(VideoConfigBuffer * buffer) = 0;
    virtual void stop(void) = 0;
    virtual void flush(void) = 0;
    virtual void flushOutport(void) = 0;
    virtual Decode_Status decode(VideoDecodeBuffer * buffer) = 0;
    virtual const VideoRenderBuffer *getOutput(bool draining = false) = 0;
    virtual const VideoFormatInfo *getFormatInfo(void) = 0;
    virtual Decode_Status signalRenderDone(void *graphichandler) = 0;
    virtual bool checkBufferAvail() = 0;

    virtual void setXDisplay(Display * x_display) = 0;
    virtual void enableNativeBuffers(void) = 0;
    virtual Decode_Status getClientNativeWindowBuffer(void *bufferHeader, void
                                                      *nativeBufferHandle)
        = 0;
    virtual void renderDone(VideoRenderBuffer * buffer) = 0;
    virtual Decode_Status flagNativeBuffer(void *pBuffer) = 0;
    virtual void releaseLock(void) = 0;
};

#endif                          /* VIDEO_DECODER_INTERFACE_H_ */
