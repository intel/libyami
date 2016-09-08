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


#ifndef VIDEO_DECODER_INTERFACE_H_
#define VIDEO_DECODER_INTERFACE_H_
// config.h should NOT be included in header file, especially for the header file used by external

#include <VideoDecoderDefs.h>

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
    * future #decode will return YAMI_DECODE_FORMAT_CHANGE (#VIDEO_DECODE_STATUS), it will trigger
    * a reconfiguration from client. (yami will not reconfigure decoder internally to avoid mismatch between client and libyami)
    */
    virtual YamiStatus start(VideoConfigBuffer* buffer) = 0;
    /// \brief reset decoder with new configuration before decoding a new stream
    /// @param[in] buffer     new stream information to reset decoder. cached data (input data or decoded video frames) are discarded.
    virtual YamiStatus reset(VideoConfigBuffer* buffer) = 0;
    /// stop decoding and destroy sw/hw decoder context.
    virtual void stop(void) = 0;
    /// discard cached data (input data or decoded video frames), it is usually required during seek
    virtual void flush(void) = 0;
    /// continue decoding with new data in @param[in] buffer; send empty data (buffer.data=NULL, buffer.size=0) to indicate EOS
    virtual YamiStatus decode(VideoDecodeBuffer* buffer) = 0;

    ///get decoded frame from decoder.
    virtual SharedPtr<VideoFrame> getOutput() = 0;

    /** \brief retrieve updated stream information after decoder has parsed the video stream.
    * client usually calls it when libyami return YAMI_DECODE_FORMAT_CHANGE in decode().
    */
    virtual const VideoFormatInfo* getFormatInfo(void) = 0;

    /// set native display
    virtual void  setNativeDisplay( NativeDisplay * display = NULL) = 0;
    virtual void  setAllocator(SurfaceAllocator* allocator) = 0;

    /*deprecated*/
    /// lockable is set to false when seek begins and reset to true after seek is done
    /// EOS also set lockable to false
    virtual void releaseLock(bool lockable=false) = 0;

    /*deprecated*/
    ///do not use this, we will remove this in near future
    virtual VADisplay getDisplayID() = 0;
};
}
#endif                          /* VIDEO_DECODER_INTERFACE_H_ */
