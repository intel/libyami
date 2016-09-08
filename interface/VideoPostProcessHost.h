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

#ifndef VIDEO_POST_PROCESS_HOST_H_
#define VIDEO_POST_PROCESS_HOST_H_

#include <string>
#include <vector>
#include <VideoPostProcessInterface.h>

extern "C" { // for dlsym usage

/** \file VideoPostProcessHost.h
*/

/** \fn IVideoPostProcess *createVideoPostProcess(const char *mimeType)
 * \brief create encoder basing on given mimetype
*/
YamiMediaCodec::IVideoPostProcess *createVideoPostProcess(const char *mimeType);
/** \fn void releaseVideoPostProcess(IVideoPostProcess *p)
 * \brief destroy encoder
*/
void releaseVideoPostProcess(YamiMediaCodec::IVideoPostProcess * p);
/** \fn void getVideoPostProcessMimeTypes()
 * \brief return the MimeTypes enabled in the current build
*/
std::vector<std::string> getVideoPostProcessMimeTypes();

typedef YamiMediaCodec::IVideoPostProcess *(*YamiCreateVideoPostProcessFuncPtr) (const char *mimeType);
typedef void (*YamiReleaseVideoPostProcessFuncPtr)(YamiMediaCodec::IVideoPostProcess * p);
}
#endif                          /* VIDEO_POST_PROCESS_HOST_H_ */
