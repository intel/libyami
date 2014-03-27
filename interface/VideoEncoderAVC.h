/*
 *  VideoEncoderAVC.h- basic va decoder for video
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

#ifndef __VIDEO_ENCODER_AVC_H__
#define __VIDEO_ENCODER_AVC_H__

#include "VideoEncoderBase.h"

class VideoEncoderAVC:public VideoEncoderBase {

  public:
    VideoEncoderAVC();
    ~VideoEncoderAVC() {
    };

    virtual Encode_Status start();
    virtual Encode_Status getOutput(VideoEncOutputBuffer * outBuffer);

    virtual Encode_Status derivedSetParams(VideoParamConfigSet *
					   videoEncParams);
    virtual Encode_Status derivedGetParams(VideoParamConfigSet *
					   videoEncParams);
    virtual Encode_Status derivedGetConfig(VideoParamConfigSet *
					   videoEncConfig);
    virtual Encode_Status derivedSetConfig(VideoParamConfigSet *
					   videoEncConfig);

  protected:
    virtual Encode_Status sendEncodeCommand(void);

  private:
    // Local Methods
    Encode_Status getOneNALUnit(uint8_t * inBuffer, uint32_t bufSize,
				uint32_t * nalSize, uint32_t * nalType,
				uint32_t * nalOffset);
    Encode_Status getHeader(uint8_t * inBuffer, uint32_t bufSize,
			    uint32_t * headerSize);
    Encode_Status outputCodecData(VideoEncOutputBuffer * outBuffer);
    Encode_Status outputOneNALU(VideoEncOutputBuffer * outBuffer,
				bool startCode);
    Encode_Status outputLengthPrefixed(VideoEncOutputBuffer * outBuffer);

    Encode_Status renderMaxSliceSize();
    Encode_Status renderAIR();
    Encode_Status renderSequenceParams();
    Encode_Status renderPictureParams();
    Encode_Status renderSliceParams();
    int calcLevel(int numMbs);

  public:
    VideoParamsAVC mVideoParamsAVC;
    uint32_t mSliceNum;
};

#endif				/* __VIDEO_ENCODER_AVC_H__ */
