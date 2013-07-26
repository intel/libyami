/*
 INTEL CONFIDENTIAL
 Copyright 2011 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
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

