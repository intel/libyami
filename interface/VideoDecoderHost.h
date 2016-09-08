/*
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
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


#ifndef VIDEO_DECODER_HOST_H_
#define VIDEO_DECODER_HOST_H_

#include <string>
#include <vector>
#include <VideoDecoderInterface.h>

extern "C" { // for dlsym usage
/** \file VideoDecoderHost.h
*/

/** \fn IVideoDecoder *createVideoDecoder(const char *mimeType)
* \brief create a decoder basing on given mimetype
*/
YamiMediaCodec::IVideoDecoder *createVideoDecoder(const char *mimeType);
/// \brief destroy the decoder
void releaseVideoDecoder(YamiMediaCodec::IVideoDecoder * p);
/** \fn void getVideoDecoderMimeTypes()
 * \brief return the MimeTypes enabled in the current build
*/
std::vector<std::string> getVideoDecoderMimeTypes();

typedef YamiMediaCodec::IVideoDecoder *(*YamiCreateVideoDecoderFuncPtr) (const char *mimeType);
typedef void (*YamiReleaseVideoDecoderFuncPtr)(YamiMediaCodec::IVideoDecoder * p);
}
#endif                          /* VIDEO_DECODER_HOST_H_ */
