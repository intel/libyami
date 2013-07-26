/*
 INTEL CONFIDENTIAL
 Copyright 2011 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

#ifndef VIDEO_ENCODER_INTERFACE_H_
#define VIDEO_ENCODER_INTERFACE_H_

#include "VideoEncoderDef.h"

class IVideoEncoder {
public:
    virtual ~IVideoEncoder() {};
    virtual Encode_Status start(void) = 0;
    virtual Encode_Status stop(void) = 0;
    virtual void flush(void) = 0;
    virtual Encode_Status encode(VideoEncRawBuffer *inBuffer) = 0;
    virtual Encode_Status getOutput(VideoEncOutputBuffer *outBuffer) = 0;
    virtual Encode_Status getParameters(VideoParamConfigSet *videoEncParams) = 0;
    virtual Encode_Status setParameters(VideoParamConfigSet *videoEncParams) = 0;
    virtual Encode_Status getConfig(VideoParamConfigSet *videoEncConfig) = 0;
    virtual Encode_Status setConfig(VideoParamConfigSet *videoEncConfig) = 0;
    virtual Encode_Status getMaxOutSize(uint32_t *maxSize) = 0;
    virtual Encode_Status getStatistics(VideoStatistics *videoStat) = 0;
};

#endif /* VIDEO_ENCODER_INTERFACE_H_ */
