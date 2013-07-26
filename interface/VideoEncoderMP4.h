/*
 INTEL CONFIDENTIAL
 Copyright 2011 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

#ifndef __VIDEO_ENCODER__MPEG4_H__
#define __VIDEO_ENCODER__MPEG4_H__

#include "VideoEncoderBase.h"

/**
  * MPEG-4:2 Encoder class, derived from VideoEncoderBase
  */
class VideoEncoderMP4: public VideoEncoderBase {
public:
    VideoEncoderMP4();
    virtual ~VideoEncoderMP4() {};

    Encode_Status getOutput(VideoEncOutputBuffer *outBuffer);
	
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
    Encode_Status getHeaderPos(uint8_t *inBuffer, uint32_t bufSize, uint32_t *headerSize);
    Encode_Status outputConfigData(VideoEncOutputBuffer *outBuffer);
    Encode_Status renderSequenceParams();
    Encode_Status renderPictureParams();
    Encode_Status renderSliceParams();

    unsigned char mProfileLevelIndication;
    uint32_t mFixedVOPTimeIncrement;
};

#endif /* __VIDEO_ENCODER__MPEG4_H__ */
