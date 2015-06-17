/*
 *  VideoPostProcessHost.h- basic va decoder for video
 *
 *  Copyright (C) 2013 Intel Corporation
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

#ifndef VIDEO_POST_PROCESS_HOST_H_
#define VIDEO_POST_PROCESS_HOST_H_

#include "VideoPostProcessInterface.h"
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

typedef YamiMediaCodec::IVideoPostProcess *(*YamiCreateVideoPostProcessFuncPtr) (const char *mimeType);
typedef void (*YamiReleaseVideoPostProcessFuncPtr)(YamiMediaCodec::IVideoPostProcess * p);
}
#endif                          /* VIDEO_POST_PROCESS_HOST_H_ */
