/*
 *  VideoEncoderH263.h- basic va decoder for video
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

#ifndef __VIDEO_ENCODER_H263_H__
#define __VIDEO_ENCODER_H263_H__

#include "VideoEncoderBase.h"

/**
  * H.263 Encoder class, derived from VideoEncoderBase
  */
class VideoEncoderH263: public VideoEncoderBase {
public:
    VideoEncoderH263();
    virtual ~VideoEncoderH263() {};

protected:
    virtual Encode_Status sendEncodeCommand(void);
    virtual Encode_Status derivedSetParams(VideoParamConfigSet *videoEncParams) {
        return ENCODE_SUCCESS;
    }
    virtual Encode_Status derivedGetParams(VideoParamConfigSet *videoEncParams) {
        return ENCODE_SUCCESS;
    }
    virtual Encode_Status derivedGetConfig(VideoParamConfigSet *videoEncConfig) {
        return ENCODE_SUCCESS;
    }
    virtual Encode_Status derivedSetConfig(VideoParamConfigSet *videoEncConfig) {
        return ENCODE_SUCCESS;
    }

    // Local Methods
private:
    Encode_Status renderSequenceParams();
    Encode_Status renderPictureParams();
    Encode_Status renderSliceParams();
};

#endif /* __VIDEO_ENCODER_H263_H__ */

