/*
 *  decodeInputCapi.cpp - decode test input capi
 *
 *  Copyright (C) 2011-2014 Intel Corporation
 *    Author: Xin Tang<xin.t.tang@intel.com>
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

#include "decodeInputCapi.h"
#include "decodeinput.h"

using namespace YamiMediaCodec;

decodeInputHandler createDecodeInput(char *fileName)
{
    return DecodeStreamInput::create(fileName);
}

const char * getMimeType(decodeInputHandler input)
{
    if(input)
        return ((DecodeStreamInput*)input)->getMimeType();
    else
        return NULL;
}

bool inputIsEOS(decodeInputHandler input)
{
    if(input)
        return ((DecodeStreamInput*)input)->m_parseToEOS;
}

bool getNextDecodeUnit(decodeInputHandler input, VideoDecodeBuffer *inputbuffer)
{
    if(input)
        return ((DecodeStreamInput*)input)->getNextDecodeUnit(*inputbuffer);
    else
        return false;
}

void releaseDecodeInput(decodeInputHandler input)
{
    if(input)
        delete (DecodeStreamInput*)input;
}
