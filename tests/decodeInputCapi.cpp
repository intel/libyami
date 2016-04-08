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

#include "decodeInputCapi.h"
#include "decodeinput.h"

using namespace YamiMediaCodec;

DecodeInputHandler createDecodeInput(char *fileName)
{
    return DecodeInput::create(fileName);
}

const char * getMimeType(DecodeInputHandler input)
{
    if(input)
        return ((DecodeInput*)input)->getMimeType();
    else
        return NULL;
}

bool decodeInputIsEOS(DecodeInputHandler input)
{
    if(input)
        return ((DecodeInput*)input)->isEOS();
    else
        return false;
}

bool getNextDecodeUnit(DecodeInputHandler input, VideoDecodeBuffer *inputbuffer)
{
    if(input)
        return ((DecodeInput*)input)->getNextDecodeUnit(*inputbuffer);
    else
        return false;
}

void releaseDecodeInput(DecodeInputHandler input)
{
    if(input)
        delete (DecodeInput*)input;
}
