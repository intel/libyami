#ifndef __VIDEO_DECODER_CAPI_H__
#define __VIDEO_DECODER_CAPI_H__

#include "VideoDecoderDefs.h"
#include "common/common_def.h"
#include <X11/Xlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* DecodeHandler;

DecodeHandler createDecoder(const char *mimeType);

Decode_Status decodeStart(DecodeHandler p, VideoConfigBuffer *buffer);

Decode_Status decodeReset(DecodeHandler p, VideoConfigBuffer *buffer);

void decodeStop(DecodeHandler p);

void decodeFlush(DecodeHandler p);

Decode_Status decode(DecodeHandler p, VideoDecodeBuffer *buffer);

const VideoRenderBuffer* decodeGetOutput(DecodeHandler p, bool draining);

Decode_Status decodeGetOutput_x11(DecodeHandler p, Drawable draw, int64_t *timeStamp
        , int drawX, int drawY, int drawWidth, int drawHeight, bool draining
        , int frameX, int frameY, int frameWidth, int frameHeight);

const VideoFormatInfo* getFormatInfo(DecodeHandler p);

void renderDone(DecodeHandler p, VideoRenderBuffer* buffer);

void decodeSetNativeDisplay(DecodeHandler p, NativeDisplay * display);

void flushOutport(DecodeHandler p);

void enableNativeBuffers(DecodeHandler p);

Decode_Status getClientNativeWindowBuffer(DecodeHandler p, void *bufferHeader, void *nativeBufferHandle);

Decode_Status flagNativeBuffer(DecodeHandler p, void * pBuffer);

void releaseLock(DecodeHandler p);

void releaseDecoder(DecodeHandler p);

#ifdef __cplusplus
};
#endif

#endif
