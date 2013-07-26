/* INTEL CONFIDENTIAL
* Copyright (c) 2009 Intel Corporation.  All rights reserved.
*
* The source code contained or described herein and all documents
* related to the source code ("Material") are owned by Intel
* Corporation or its suppliers or licensors.  Title to the
* Material remains with Intel Corporation or its suppliers and
* licensors.  The Material contains trade secrets and proprietary
* and confidential information of Intel or its suppliers and
* licensors. The Material is protected by worldwide copyright and
* trade secret laws and treaty provisions.  No part of the Material
* may be used, copied, reproduced, modified, published, uploaded,
* posted, transmitted, distributed, or disclosed in any way without
* Intel's prior express written permission.
*
* No license under any patent, copyright, trade secret or other
* intellectual property right is granted to or conferred upon you
* by disclosure or delivery of the Materials, either expressly, by
* implication, inducement, estoppel or otherwise. Any license
* under such intellectual property rights must be express and
* approved by Intel in writing.
*
*/


#ifndef VIDEO_DECODER_INTERFACE_H_
#define VIDEO_DECODER_INTERFACE_H_

#include "VideoDecoderDefs.h"

class IVideoDecoder {
public:
    virtual ~IVideoDecoder() {}
    virtual Decode_Status start(VideoConfigBuffer *buffer) = 0;
    virtual Decode_Status reset(VideoConfigBuffer *buffer) = 0;
    virtual void stop(void) = 0;
    virtual void flush(void) = 0;
    virtual void flushOutport(void) = 0;
    virtual Decode_Status decode(VideoDecodeBuffer *buffer) = 0;
    virtual const VideoRenderBuffer* getOutput(bool draining = false) = 0;
    virtual const VideoFormatInfo* getFormatInfo(void) = 0;
    virtual Decode_Status signalRenderDone(void * graphichandler) = 0;
    virtual bool checkBufferAvail() = 0;

    virtual void  enableNativeBuffers(void) = 0;
    virtual Decode_Status  getClientNativeWindowBuffer(void *bufferHeader, void *nativeBufferHandle) = 0;
    virtual void renderDone(VideoRenderBuffer* buffer) = 0;
    virtual Decode_Status flagNativeBuffer(void * pBuffer) = 0;
    virtual void releaseLock(void) = 0;
};

#endif /* VIDEO_DECODER_INTERFACE_H_ */
