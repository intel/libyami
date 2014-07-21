/*
 *  VideoEncoderInterface.h- basic va encoder for video
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

#ifndef VIDEO_ENCODER_INTERFACE_H_
#define VIDEO_ENCODER_INTERFACE_H_

#include "VideoEncoderDef.h"
#include <X11/Xlib.h>
#undef None // work around for compile in chromeos

namespace YamiMediaCodec{
/**
 * \class IVideoEncoder
 * \brief Abstract video encoding interface of libyami
 * it is the interface with client
 */
class IVideoEncoder {
  public:
    virtual ~ IVideoEncoder() {}
    ///set external display
    virtual void  setXDisplay(Display * xdisplay) = 0;
    /** \brief start encode, make sure you setParameters before start, else if will use default.
    */
    virtual Encode_Status start(void) = 0;
    /// stop encoding and destroy encoder context.
    virtual Encode_Status stop(void) = 0;

    /// continue encoding with new data in @param[in] inBuffer
    virtual Encode_Status encode(VideoEncRawBuffer * inBuffer) = 0;
    /**
     * \brief return one frame encoded data to client;
     * ENCODE_BUFFER_NO_MORE will be returned if no available frame
     */
    virtual Encode_Status getOutput(VideoEncOutputBuffer * outBuffer) = 0;
    /// get encoder params
    virtual Encode_Status getParameters(VideoParamConfigSet * videoEncParams) = 0;
    /// set encoder params
    virtual Encode_Status setParameters(VideoParamConfigSet * videoEncParams) = 0;
    /// get max coded buffer size.
    virtual Encode_Status getMaxOutSize(uint32_t * maxSize) = 0;

    /// get encode statistics information, for debug use
    virtual Encode_Status getStatistics(VideoStatistics * videoStat) = 0;

    ///obsolete, discard cached data (input data or encoded video frames), not sure why an encoder need this
    virtual void flush(void) = 0;
    ///obsolete, what is the difference between  getParameters and getConfig?
    virtual Encode_Status getConfig(VideoParamConfigSet * videoEncConfig) = 0;
    ///obsolete, what is the difference between  setParameters and setConfig?
    virtual Encode_Status setConfig(VideoParamConfigSet * videoEncConfig) = 0;

};
}
#endif                          /* VIDEO_ENCODER_INTERFACE_H_ */
