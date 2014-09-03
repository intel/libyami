#include "VideoDecoderCapi.h"
#include "VideoDecoderInterface.h"
#include "VideoDecoderHost.h"

using namespace YamiMediaCodec;

DecodeHandler createDecoder(const char *mimeType)
{
    return createVideoDecoder(mimeType);
}

Decode_Status decodeStart(DecodeHandler p, VideoConfigBuffer *buffer)
{
    if(p)
        return ((IVideoDecoder*)p)->start(buffer);
    else
        return DECODE_FAIL;
}

Decode_Status decodeReset(DecodeHandler p, VideoConfigBuffer *buffer)
{
     if(p)
        return ((IVideoDecoder*)p)->reset(buffer);
     else
        return DECODE_FAIL;
}

void decodeStop(DecodeHandler p)
{
     if(p)
        ((IVideoDecoder*)p)->stop();
}

void decodeFlush(DecodeHandler p)
{
     if(p)
        ((IVideoDecoder*)p)->flush();
}

Decode_Status decode(DecodeHandler p, VideoDecodeBuffer *buffer)
{
     if(p)
        return ((IVideoDecoder*)p)->decode(buffer);
     else
        return DECODE_FAIL;
}

const VideoRenderBuffer* decodeGetOutput(DecodeHandler p, bool draining)
{
     if(p)
        return ((IVideoDecoder*)p)->getOutput(draining);
}

Decode_Status decodeGetOutput_x11(DecodeHandler p, Drawable draw, int64_t *timeStamp
        , int drawX, int drawY, int drawWidth, int drawHeight, bool draining
        , int frameX, int frameY, int frameWidth, int frameHeight)
{
    if(p)
        return ((IVideoDecoder*)p)->getOutput(draw, timeStamp, drawX, drawY, drawWidth, drawHeight, draining
                        , frameX, frameY, frameWidth, frameHeight);
    else
        return DECODE_FAIL;
}

const VideoFormatInfo* getFormatInfo(DecodeHandler p)
{
    if(p)
        return ((IVideoDecoder*)p)->getFormatInfo();
}

void renderDone(DecodeHandler p, VideoRenderBuffer* buffer)
{
    if(p)
        ((IVideoDecoder*)p)->renderDone(buffer);
}

void decodeSetNativeDisplay(DecodeHandler p, NativeDisplay * display)
{
    if(p)
        ((IVideoDecoder*)p)->setNativeDisplay(display);
}

void flushOutport(DecodeHandler p)
{
    if(p)
        ((IVideoDecoder*)p)->flushOutport();
}

void enableNativeBuffers(DecodeHandler p)
{
    if(p)
        ((IVideoDecoder*)p)->enableNativeBuffers();
}

Decode_Status getClientNativeWindowBuffer(DecodeHandler p, void *bufferHeader, void *nativeBufferHandle)
{
    if(p)
        return ((IVideoDecoder*)p)->getClientNativeWindowBuffer(bufferHeader, nativeBufferHandle);
    else
        return DECODE_FAIL;
}

Decode_Status flagNativeBuffer(DecodeHandler p, void * pBuffer)
{
    if(p)
        return ((IVideoDecoder*)p)->flagNativeBuffer(pBuffer);
    else
        return DECODE_FAIL;
}

void releaseLock(DecodeHandler p)
{
    if(p)
        ((IVideoDecoder*)p)->releaseLock();
}

void releaseDecoder(DecodeHandler p)
{
    if(p)
        releaseVideoDecoder((IVideoDecoder*)p);
}
