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

namespace YamiMediaCodec {
/**
 * \class IVideoDecoder
 * \brief Abstract video decoding interface of libyami
 *
 * it is the interface with client
 */
class IVideoDecoder {
public:
    virtual ~IVideoDecoder() {}
    /** \brief configure decoder before decode the first frame
    * @param[in] buffer     stream information to initialize decoder
    * if the configuration in @param buffer doesn't match the information in coming stream,
    * future #decode will return DECODE_FORMAT_CHANGE (#VIDEO_DECODE_STATUS), it will trigger
    * a reconfiguration from client. (yami will not reconfigure decoder internally to avoid mismatch between client and libyami)
    */
    virtual Decode_Status start(VideoConfigBuffer *buffer) = 0;
    /// \brief reset decoder with new configuration before decoding a new stream
    /// @param[in] buffer     new stream information to reset decoder. cached data (input data or decoded video frames) are discarded.
    virtual Decode_Status reset(VideoConfigBuffer *buffer) = 0;
    /// stop decoding and destroy sw/hw decoder context.
    virtual void stop(void) = 0;
    /// discard cached data (input data or decoded video frames), it is usually required during seek
    virtual void flush(void) = 0;
    /// continue decoding with new data in @param[in] buffer
    virtual Decode_Status decode(VideoDecodeBuffer *buffer) = 0;
    /**
     * \brief return one frame to client for display;
     * NULL will be returned if no available frame for rendering
     * @param[in] draining drain out all possible frames or not. it is set to true upon EOS.
     * @return a #VideoRenderBuffer to be rendered by client
     */
    virtual const VideoRenderBuffer* getOutput(bool draining = false) = 0;
    /**
     * \brief  render one available video frame to draw
     * @param[in] draw a X11 drawable, Pixmap or Window ID
     * @param[in] drawX/drawY/drawWidth/drawHeight specify a rect to render on the draw
     * @param[in] drawX horizontal offset to render on the draw
     * @param[in] drawY vertical offset to render on the draw
     * @param[in] width rendering rect width on the draw
     * @param[in] height render rect height on the draw
     * @param[in] frameX/frameY/frameWidth/frameHeight specify a portion rect for rendering, default means the whole surface to render
     * @param[in] frameX horizontal offset of the video frame to render
     * @param[in] frameY vertical offset of the video frame to render
     * @param[in] frameWidth rect width of the video frame to render
     * @param[in] frameHeight rect height of the video frame to render
     *
     * default value of frameX/frameY/frameWidth/frameHeight means rendering the whole video frame(surface)
     *
     * @return DECODE_SUCCESS for success
     * @return RENDER_NO_AVAILABLE_FRAME when no available frame to render
     * @return RENDER_FAIL when driver fail to do vaPutSurface
     * @return RENDER_INVALID_PARAMETER
     */
    virtual Decode_Status getOutput(Drawable draw, int drawX, int drawY, int drawWidth, int drawHeight, bool draining = false,
        int frameX = -1, int frameY = -1, int frameWidth = -1, int frameHeight = -1) = 0;

    /** \brief retrieve updated stream information after decoder has parsed the video stream.
    * client usually calls it when libyami return DECODE_FORMAT_CHANGE in decode().
    */
    virtual const VideoFormatInfo* getFormatInfo(void) = 0;
    /** \brief client recycles buffer back to libyami after the buffer has been rendered.
    *
    * <pre>
    * it is used by current omx client in async rendering mode.
    * if rendering target is passed in getOutput(), then yami can does the rendering directly; this function becomes unnecessary.
    * however, this API is still useful when we export video frame directly for EGLImage (dma_buf).
    * </pre>
    */
    virtual void renderDone(VideoRenderBuffer* buffer) = 0;

    /// todo: move the x_display to VideoConfigBuffer and mark this API as obsolete
    virtual void  setXDisplay(Display * x_display) = 0;
    /// obsolete, make all cached video frame output-able, it can be done by getOutput(draining=true) as well
    virtual void flushOutport(void) = 0;
    /// obsolete, usually client needn't check the available of output frame but blindly calls getOutput.
    virtual bool checkBufferAvail() = 0;
    /// not interest for now, may be used by Android
    virtual Decode_Status signalRenderDone(void * graphichandler) = 0;
    /// not interest for now, may be used by Android to accept external video frame memory from gralloc
    virtual void  enableNativeBuffers(void) = 0;
    /// not interest for now, may be used by Android to accept external video frame memory from gralloc
    virtual Decode_Status  getClientNativeWindowBuffer(void *bufferHeader, void *nativeBufferHandle) = 0;
    /// not interest for now, may be used by Android to accept external video frame memory from gralloc
    virtual Decode_Status flagNativeBuffer(void * pBuffer) = 0;
    /// not interest for now, may be used by Android
    virtual void releaseLock(void) = 0;
};
}
#endif                          /* VIDEO_DECODER_INTERFACE_H_ */
