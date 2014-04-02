/*
 *  VideoEncoderBase.h- basic va decoder for video
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

#ifndef __VIDEO_ENC_BASE_H__
#define __VIDEO_ENC_BASE_H__

#include <va/va.h>
#include "VideoEncoderDef.h"
#include "VideoEncoderInterface.h"

class VideoEncoderBase:IVideoEncoder {

  public:
    VideoEncoderBase();
    virtual ~ VideoEncoderBase();

    virtual Encode_Status start(void);
    virtual void flush(void);
    virtual Encode_Status stop(void);
    virtual Encode_Status encode(VideoEncRawBuffer * inBuffer);

    /*
     * getOutput can be called several time for a frame (such as first time  codec data, and second time others)
     * encode will provide encoded data according to the format (whole frame, codec_data, sigle NAL etc)
     * If the buffer passed to encoded is not big enough, this API call will return ENCODE_BUFFER_TOO_SMALL
     * and caller should provide a big enough buffer and call again
     */
    virtual Encode_Status getOutput(VideoEncOutputBuffer * outBuffer);


    virtual Encode_Status getParameters(VideoParamConfigSet *
                                        videoEncParams);
    virtual Encode_Status setParameters(VideoParamConfigSet *
                                        videoEncParams);
    virtual Encode_Status setConfig(VideoParamConfigSet * videoEncConfig);
    virtual Encode_Status getConfig(VideoParamConfigSet * videoEncConfig);

    virtual Encode_Status getMaxOutSize(uint32_t * maxSize);


  protected:
     virtual Encode_Status sendEncodeCommand(void) = 0;
    virtual Encode_Status derivedSetParams(VideoParamConfigSet *
                                           videoEncParams) = 0;
    virtual Encode_Status derivedGetParams(VideoParamConfigSet *
                                           videoEncParams) = 0;
    virtual Encode_Status derivedGetConfig(VideoParamConfigSet *
                                           videoEncConfig) = 0;
    virtual Encode_Status derivedSetConfig(VideoParamConfigSet *
                                           videoEncConfig) = 0;

    Encode_Status prepareForOutput(VideoEncOutputBuffer * outBuffer,
                                   bool * useLocalBuffer);
    Encode_Status cleanupForOutput();
    Encode_Status outputAllData(VideoEncOutputBuffer * outBuffer);
    Encode_Status renderDynamicFrameRate();
    Encode_Status renderDynamicBitrate();

  private:
    void setDefaultParams(void);
    Encode_Status setUpstreamBuffer(VideoBufferSharingMode bufferMode,
                                    uint32_t * bufList, uint32_t bufCnt);
    Encode_Status getNewUsrptrFromSurface(uint32_t width, uint32_t height,
                                          uint32_t format,
                                          uint32_t expectedSize,
                                          uint32_t * outsize,
                                          uint32_t * stride,
                                          uint8_t ** usrptr);

    VideoEncSurfaceBuffer *appendVideoSurfaceBuffer(VideoEncSurfaceBuffer *
                                                    head,
                                                    VideoEncSurfaceBuffer *
                                                    buffer);
    VideoEncSurfaceBuffer *removeVideoSurfaceBuffer(VideoEncSurfaceBuffer *
                                                    head,
                                                    VideoEncSurfaceBuffer *
                                                    buffer);
     VideoEncSurfaceBuffer
        * getVideoSurfaceBufferByIndex(VideoEncSurfaceBuffer * head,
                                       uint32_t index);

    Encode_Status manageSrcSurface(VideoEncRawBuffer * inBuffer);
    void updateProperities(void);
    void decideFrameType(void);
    Encode_Status uploadDataToSurface(VideoEncRawBuffer * inBuffer);

  protected:
    bool mInitialized;
    VADisplay mVADisplay;
    VAContextID mVAContext;
    VAConfigID mVAConfig;
    VAEntrypoint mVAEntrypoint;

    VACodedBufferSegment *mCurSegment;
    uint32_t mOffsetInSeg;
    uint32_t mTotalSize;
    uint32_t mTotalSizeCopied;

    VideoParamsCommon mComParams;

    VideoBufferSharingMode mBufferMode;
    uint32_t *mUpstreamBufferList;
    uint32_t mUpstreamBufferCnt;

    bool mForceKeyFrame;
    bool mNewHeader;
    bool mFirstFrame;

    bool mRenderMSS;            //Max Slice Size
    bool mRenderQP;
    bool mRenderAIR;
    bool mRenderFrameRate;
    bool mRenderBitRate;

    VABufferID mVACodedBuffer[2];
    VABufferID mLastCodedBuffer;
    VABufferID mOutCodedBuffer;
    VABufferID mSeqParamBuf;
    VABufferID mPicParamBuf;
    VABufferID mSliceParamBuf;

    VASurfaceID *mSharedSurfaces;
    VASurfaceID *mSurfaces;
    uint32_t mSurfaceCnt;
    uint32_t mSharedSurfacesCnt;
    uint32_t mReqSurfacesCnt;
    uint8_t **mUsrPtr;

    VideoEncSurfaceBuffer *mVideoSrcBufferList;
    VideoEncSurfaceBuffer *mCurFrame;   //current input frame to be encoded;
    VideoEncSurfaceBuffer *mRefFrame;   //reference frame
    VideoEncSurfaceBuffer *mRecFrame;   //reconstructed frame;
    VideoEncSurfaceBuffer *mLastFrame;  //last frame;

    VideoEncRawBuffer *mLastInputRawBuffer;

    uint32_t mEncodedFrames;
    uint32_t mFrameNum;
    uint32_t mCodedBufSize;
    uint32_t mCodedBufIndex;

    bool mPicSkipped;
    bool mIsIntra;
    bool mSliceSizeOverflow;
    bool mCodedBufferMapped;
    bool mDataCopiedOut;
    bool mKeyFrame;
};

#endif                          /* __MIX_VIDEO_ENC_BASE_H__ */
