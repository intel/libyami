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
// config.h should NOT be included in header file, especially for the header file used by external

#include "VideoEncoderDefs.h"
#undef None // work around for compile in chromeos

namespace YamiMediaCodec{
/**
 * \class IVideoEncoder
 * \brief Abstract video encoding interface of libyami
 * it is the interface with client. they are not thread safe.
 * getOutput() can be called in one (and only one) separated thread, though IVideoEncoder APIs are not fully thread safe.
 */
class IVideoEncoder {
  public:
    virtual ~ IVideoEncoder() {}
    /// set native display
    virtual void  setNativeDisplay( NativeDisplay * display = NULL) = 0;
    /** \brief start encode, make sure you setParameters before start, else if will use default.
    */
    virtual Encode_Status start(void) = 0;
    /// stop encoding and destroy encoder context.
    virtual Encode_Status stop(void) = 0;

    /// continue encoding with new data in @param[in] inBuffer
    virtual Encode_Status encode(VideoEncRawBuffer * inBuffer) = 0;
    /**
     * \brief return one frame encoded data to client;
     * when withWait is false, ENCODE_BUFFER_NO_MORE will be returned if there is no available frame. \n
     * when withWait is true, function call is block until there is one frame available. \n
     * typically, getOutput() is called in a separate thread (than encoding thread), this thread sleeps when
     * there is no output available when withWait is true. \n
     *
     * param [in/out] outBuffer a #VideoEncOutputBuffer of one frame encoded data
     * param [in/out] when there is no output data available, wait or not
     */
    virtual Encode_Status getOutput(VideoEncOutputBuffer * outBuffer, bool withWait = false) = 0;
    /// get encoder params, some config parameter are updated basing on sw/hw implement limition.
    /// for example, update pitches basing on hw alignment
    virtual Encode_Status getParameters(VideoParamConfigType type, Yami_PTR videoEncParams) = 0;
    /// set encoder params before start. \n
    /// update rate controls on the fly by VideoParamsTypeRatesControl/VideoParamsRatesControl. SPS updates accordingly.
    virtual Encode_Status setParameters(VideoParamConfigType type, Yami_PTR videoEncParams) = 0;
    /// get max coded buffer size.
    virtual Encode_Status getMaxOutSize(uint32_t * maxSize) = 0;

    /// get encode statistics information, for debug use
    virtual Encode_Status getStatistics(VideoStatistics * videoStat) = 0;

    ///obsolete, discard cached data (input data or encoded video frames), not sure why an encoder need this
    virtual void flush(void) = 0;
    ///obsolete, what is the difference between  getParameters and getConfig?
    virtual Encode_Status getConfig(VideoParamConfigType type, Yami_PTR videoEncConfig) = 0;
    ///obsolete, what is the difference between  setParameters and setConfig?
    virtual Encode_Status setConfig(VideoParamConfigType type, Yami_PTR videoEncConfig) = 0;

};
}
#endif                          /* VIDEO_ENCODER_INTERFACE_H_ */
