/*
 *  decodeinput.h - decode test input
 *
 *  Copyright (C) 2011-2014 Intel Corporation
 *    Author: Changzhi Wei<changzhix.wei@intel.com>
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
#ifndef decodeinput_h
#define decodeinput_h

#include <stdio.h>
#include <string>
#include "VideoDecoderDefs.h"
#include "VideoDecoderInterface.h"

using std::string;
class DecodeInput {
public:
    virtual ~DecodeInput() {}
    static DecodeInput * create(const char* fileName);
    virtual bool isEOS() = 0;
    virtual const char * getMimeType() = 0;
    virtual bool getNextDecodeUnit(VideoDecodeBuffer &inputBuffer) = 0;
    virtual const string& getCodecData() = 0;

protected:
    virtual bool initInput(const char* fileName) = 0;

};
#endif

