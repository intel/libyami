/*
 *  VideoEncoderInterface.h- basic va decoder for video
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

class IVideoEncoder {
  public:
    virtual ~ IVideoEncoder() {
    };
    virtual Encode_Status start(void) = 0;
    virtual Encode_Status stop(void) = 0;
    virtual void flush(void) = 0;
    virtual Encode_Status encode(VideoEncRawBuffer * inBuffer) = 0;
    virtual Encode_Status getOutput(VideoEncOutputBuffer * outBuffer) = 0;
    virtual Encode_Status getParameters(VideoParamConfigSet *
					videoEncParams) = 0;
    virtual Encode_Status setParameters(VideoParamConfigSet *
					videoEncParams) = 0;
    virtual Encode_Status getConfig(VideoParamConfigSet * videoEncConfig) =
	0;
    virtual Encode_Status setConfig(VideoParamConfigSet * videoEncConfig) =
	0;
    virtual Encode_Status getMaxOutSize(uint32_t * maxSize) = 0;
    virtual Encode_Status getStatistics(VideoStatistics * videoStat) = 0;
};

#endif				/* VIDEO_ENCODER_INTERFACE_H_ */
