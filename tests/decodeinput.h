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
#ifndef decodeinput_h
#define decodeinput_h

#include <stdio.h>
#include <string>
#include "VideoDecoderDefs.h"
#include "VideoDecoderInterface.h"

using std::string;
class DecodeInput {
public:
    DecodeInput();
    virtual ~DecodeInput() {}
    static DecodeInput * create(const char* fileName);
    virtual bool isEOS() = 0;
    virtual const char * getMimeType() = 0;
    virtual bool getNextDecodeUnit(VideoDecodeBuffer &inputBuffer) = 0;
    virtual const string& getCodecData() = 0;
    virtual uint16_t getWidth() {return m_width;}
    virtual uint16_t getHeight() {return m_height;}

protected:
    virtual bool initInput(const char* fileName) = 0;
    virtual void setResolution(const uint16_t width, const uint16_t height);
    uint16_t m_width;
    uint16_t m_height;

};
#endif

