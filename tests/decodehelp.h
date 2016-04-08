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
#ifndef decodehelp_h
#define decodehelp_h

#include <stdint.h>
#include <string>

typedef struct DecodeParameter {
    char* inputFile;
    int width;
    int height;
    short renderMode;
    short waitBeforeQuit;
    uint32_t renderFrames;
    uint32_t renderFourcc;
    std::string outputFile;
} DecodeParameter;

bool processCmdLine(int argc, char** argv, DecodeParameter* parameters);

bool possibleWait(const char* mimeType, const DecodeParameter* parameters);

#endif //decodehelp_h
