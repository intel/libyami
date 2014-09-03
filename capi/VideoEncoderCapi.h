#ifndef __VIDEO_ENCODER_CAPI_H__
#define __VIDEO_ENCODER_CAPI_H__

#include "VideoEncoderDefs.h"
#include "common/common_def.h"
#include <X11/Xlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* EncodeHandler;

EncodeHandler createEncoder(const char *mimeType);

void encodeSetNativeDisplay(EncodeHandler p, NativeDisplay * display);

Encode_Status encodeStart(EncodeHandler p);

void encodeStop(EncodeHandler p);

void encodeflush(EncodeHandler p);

Encode_Status encode(EncodeHandler p, VideoEncRawBuffer * inBuffer);

Encode_Status encodeGetOutput(EncodeHandler p, VideoEncOutputBuffer * outBuffer, bool withWait);

Encode_Status getParameters(EncodeHandler p, VideoParamConfigType type, Yami_PTR videoEncParams);

Encode_Status setParameters(EncodeHandler p, VideoParamConfigType type, Yami_PTR videoEncParams);

Encode_Status getMaxOutSize(EncodeHandler p, uint32_t * maxSize);

Encode_Status getStatistics(EncodeHandler p, VideoStatistics * videoStat);

Encode_Status getConfig(EncodeHandler p, VideoParamConfigType type, Yami_PTR videoEncConfig);

Encode_Status setConfig(EncodeHandler p, VideoParamConfigType type, Yami_PTR videoEncConfig);

void releaseEncoder(EncodeHandler p);

#ifdef __cplusplus
};
#endif

#endif
