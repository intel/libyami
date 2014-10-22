/*
 *  decodeOutputCapi.h - c api helper for decdoe output
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

#include "VideoDecoderDefs.h"
#include "capi/VideoDecoderCapi.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DecodeOutputTag
{
};

typedef struct DecodeOutputTag* DecodeOutputHandler;



DecodeOutputHandler createDecodeOutput(DecodeHandler decoder, int renderMode);
bool configDecodeOutput(DecodeOutputHandler output);
bool decodeOutputSetVideoSize(DecodeOutputHandler output, int width , int height);
bool renderOutputFrames(DecodeOutputHandler output, bool drain);
void releaseDecodeOutput(DecodeOutputHandler output);




#ifdef __cplusplus
}
#endif
