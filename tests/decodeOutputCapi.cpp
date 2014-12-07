/*
 *  decodeOutputCapi.cpp - c api helper for decdoe output
 *
 *  Copyright (C) 2011-2014 Intel Corporation
 *    Author: Xu Guangxin <guangxin.xu@intel.com>
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

bool renderOutputFrames(DecodeOutputHandler output, bool drain)
{
    if (!output)
        return false;
    DecodeOutput* out = reinterpret_cast<DecodeOutput*>(output);
    return renderOutputFrames(out, drain);
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
