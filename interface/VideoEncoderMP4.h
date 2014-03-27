/*
 *  VideoEncoderMP4.h- basic va decoder for video
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

#ifndef __VIDEO_ENCODER__MPEG4_H__
#define __VIDEO_ENCODER__MPEG4_H__

#include "VideoEncoderBase.h"

/**
  * MPEG-4:2 Encoder class, derived from VideoEncoderBase
  */
class VideoEncoderMP4:public VideoEncoderBase {
  public:
    VideoEncoderMP4();
    virtual ~ VideoEncoderMP4() {
    };

    Encode_Status getOutput(VideoEncOutputBuffer * outBuffer);

  protected:
    virtual Encode_Status sendEncodeCommand(void);

    virtual Encode_Status derivedSetParams(VideoParamConfigSet *
					   videoEncParams) {
	return ENCODE_SUCCESS;
    }
    virtual Encode_Status derivedGetParams(VideoParamConfigSet *
					   videoEncParams) {
	return ENCODE_SUCCESS;
    }
    virtual Encode_Status derivedGetConfig(VideoParamConfigSet *
					   videoEncConfig) {
	return ENCODE_SUCCESS;
    }
    virtual Encode_Status derivedSetConfig(VideoParamConfigSet *
					   videoEncConfig) {
	return ENCODE_SUCCESS;
    }
    // Local Methods
  private:
    Encode_Status getHeaderPos(uint8_t * inBuffer, uint32_t bufSize,
			       uint32_t * headerSize);
    Encode_Status outputConfigData(VideoEncOutputBuffer * outBuffer);
    Encode_Status renderSequenceParams();
    Encode_Status renderPictureParams();
    Encode_Status renderSliceParams();

    unsigned char mProfileLevelIndication;
    uint32_t mFixedVOPTimeIncrement;
};

#endif				/* __VIDEO_ENCODER__MPEG4_H__ */
