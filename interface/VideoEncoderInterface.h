/*
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef VIDEO_ENCODER_INTERFACE_H_
#define VIDEO_ENCODER_INTERFACE_H_
// config.h should NOT be included in header file, especially for the header file used by external

#include <VideoEncoderDefs.h>
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
    virtual YamiStatus start(void) = 0;
    /// stop encoding and destroy encoder context.
    virtual YamiStatus stop(void) = 0;

    /// continue encoding with new data in @param[in] inBuffer
    virtual YamiStatus encode(VideoEncRawBuffer* inBuffer) = 0;
    /// continue encoding with new data in @param[in] frame
    virtual YamiStatus encode(VideoFrameRawData* frame) = 0;

    /// continue encoding with new data in @param[in] frame
    /// we will hold a reference of @param[in]frame, until encode is done
    virtual YamiStatus encode(const SharedPtr<VideoFrame>& frame) = 0;

#ifndef __BUILD_GET_MV__
    /**
     * \brief return one frame encoded data to client;
     * when withWait is false, YAMI_ENCODE_BUFFER_NO_MORE will be returned if there is no available frame. \n
     * when withWait is true, function call is block until there is one frame available. \n
     * typically, getOutput() is called in a separate thread (than encoding thread), this thread sleeps when
     * there is no output available when withWait is true. \n
     *
     * param [in/out] outBuffer a #VideoEncOutputBuffer of one frame encoded data
     * param [in/out] when there is no output data available, wait or not
     */
    virtual YamiStatus getOutput(VideoEncOutputBuffer* outBuffer, bool withWait = false) = 0;
#else
    /**
     * \brief return one frame encoded data to client;
     * when withWait is false, YAMI_ENCODE_BUFFER_NO_MORE will be returned if there is no available frame. \n
     * when withWait is true, function call is block until there is one frame available. \n
     * typically, getOutput() is called in a separate thread (than encoding thread), this thread sleeps when
     * there is no output available when withWait is true. \n
     *
     * param [in/out] outBuffer a #VideoEncOutputBuffer of one frame encoded data
     * param [in/out] MVBuffer  a #VideoEncMVBuffer of one frame MV data
     * param [in/out] when there is no output data available, wait or not
     */
    virtual YamiStatus getOutput(VideoEncOutputBuffer* outBuffer, VideoEncMVBuffer* MVBuffer, bool withWait = false) = 0;
#endif

    /// get encoder params, some config parameter are updated basing on sw/hw implement limition.
    /// for example, update pitches basing on hw alignment
    virtual YamiStatus getParameters(VideoParamConfigType type, Yami_PTR videoEncParams) = 0;
    /// set encoder params before start. \n
    /// update rate controls on the fly by VideoParamsTypeRatesControl/VideoParamsRatesControl. SPS updates accordingly.
    virtual YamiStatus setParameters(VideoParamConfigType type, Yami_PTR videoEncParams) = 0;
    /// get max coded buffer size.
    virtual YamiStatus getMaxOutSize(uint32_t* maxSize) = 0;

#ifdef __BUILD_GET_MV__
    /// get MV buffer size.
    virtual YamiStatus getMVBufferSize(uint32_t* Size) = 0;
#endif

    /// get encode statistics information, for debug use
    virtual YamiStatus getStatistics(VideoStatistics* videoStat) = 0;

    ///flush cached data (input data or encoded video frames)
    virtual void flush(void) = 0;
    ///obsolete, what is the difference between  getParameters and getConfig?
    virtual YamiStatus getConfig(VideoParamConfigType type, Yami_PTR videoEncConfig) = 0;
    ///obsolete, what is the difference between  setParameters and setConfig?
    virtual YamiStatus setConfig(VideoParamConfigType type, Yami_PTR videoEncConfig) = 0;
};
}
#endif                          /* VIDEO_ENCODER_INTERFACE_H_ */
