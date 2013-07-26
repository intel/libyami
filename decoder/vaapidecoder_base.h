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

#ifndef VAAPI_DECODER_BASE_H_
#define VAAPI_DECODER_BASE_H_

#include <pthread.h>
#include <va/va.h>
#include <va/va_tpi.h>
#include "interface/VideoDecoderInterface.h"
#include "vaapisurfacebuf_pool.h"
#include "common/log.h"

#ifndef Display
typedef unsigned int Display;
#endif

#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#define ALIGN_MB(a) (((a) + 15 ) & (~15))

class VaapiDecoderBase : public IVideoDecoder {
public:
    VaapiDecoderBase(const char *mimeType);
    virtual ~VaapiDecoderBase();

    virtual Decode_Status start(VideoConfigBuffer *buffer);
    virtual Decode_Status reset(VideoConfigBuffer *buffer) ;
    virtual void stop(void);
    //virtual Decode_Status decode(VideoDecodeBuffer *buffer);
    virtual void flush(void);
    virtual void flushOutport(void);
    virtual const VideoRenderBuffer* getOutput(bool draining = false);
    virtual Decode_Status signalRenderDone(void * graphichandler);
    virtual const VideoFormatInfo* getFormatInfo(void);
    virtual bool checkBufferAvail(void);
    virtual void renderDone(VideoRenderBuffer *renderBuf);

    /* native window related functions */
    void enableNativeBuffers(void);
    Decode_Status getClientNativeWindowBuffer(void *bufferHeader,
                  void *nativeBufferHandle);
    Decode_Status flagNativeBuffer(void *pBuffer);
    void releaseLock();

protected:
    Decode_Status setupVA(uint32_t numSurface, VAProfile profile);
    Decode_Status terminateVA(void);
    Decode_Status updateReference(void);

    Display     *mDisplay;
    VADisplay    mVADisplay;
    VAContextID  mVAContext;
    VAConfigID   mVAConfig;
    bool         mVAStarted;
 
    VideoConfigBuffer mConfigBuffer;
    VideoFormatInfo mVideoFormatInfo;

    /* allocate all surfaces need for decoding & display
     * in one pool, the pool will responsable for allocating
     * empty surface, recycle used surface.
     */
    VaapiSurfaceBufferPool* mBufPool;
    /* the current render target for decoder */
    VideoSurfaceBuffer *mRenderTarget;

    /* reference picture, h264 will not use */
    VideoSurfaceBuffer *mLastReference;
    VideoSurfaceBuffer *mForwardReference;
    
    /* hold serveral decoded picture coming from the dpb,
     * and rearrange the picture output order according to
     * customer required, for example, when output
     */
    //VideoSurfaceBuffer *outputList;
    uint64_t mCurrentPTS; 
 
private:
    bool mLowDelay;
    bool mRawOutput;
    bool mEnableNativeBuffersFlag;
};

#endif  // VIDEO_DECODER_BASE_H_
