/*
 * Copyright (C) 2011-2014 Intel Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "decodeOutputCapi.h"
#include "interface/VideoDecoderInterface.h"
#include "decodeoutput.h"

using namespace YamiMediaCodec;

DecodeOutputHandler createDecodeOutput(DecodeHandler decoder, int renderMode)
{
    if (!decoder)
        return NULL;
    IVideoDecoder* dec = reinterpret_cast<IVideoDecoder*>(decoder);
    DecodeOutput* out = DecodeOutput::create(dec, renderMode);
    return reinterpret_cast<DecodeOutputHandler>(out);
}

bool decodeOutputSetVideoSize(DecodeOutputHandler output, int width , int height)
{
    if (!output)
        return false;
    DecodeOutput* out = reinterpret_cast<DecodeOutput*>(output);
    return out->setVideoSize(width, height);
}

int renderOutputFrames(DecodeOutputHandler output, uint32_t maxframes, bool drain)
{
    if (!output)
        return -1;
    DecodeOutput* out = reinterpret_cast<DecodeOutput*>(output);
    if(out->renderFrameCount() != maxframes)
        renderOutputFrames(out, maxframes, drain);
    return out->renderFrameCount();
}

void releaseDecodeOutput(DecodeOutputHandler output)
{
    if (!output)
        return;
    DecodeOutput* out = reinterpret_cast<DecodeOutput*>(output);
    delete out;
}

bool configDecodeOutput(DecodeOutputHandler output)
{
    if (!output)
        return false;
    DecodeOutput* out = reinterpret_cast<DecodeOutput*>(output);
    return configDecodeOutput(out);
}
