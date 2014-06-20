/*
 *  vaapiutils.h - VA-API utilities
 *
 *  Copyright (C) 2010-2011 Splitted-Desktop Systems
 *    Author: Gwenole Beauchesne <gwenole.beauchesne@splitted-desktop.com>
 *  Copyright (C) 2011-2014 Intel Corporation
 *    Author: Gwenole Beauchesne <gwenole.beauchesne@intel.com>
 *    Author: Xiaowei Li<xiaowei.li@intel.com>
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

#ifndef vaapiutils_h
#define vaapiutils_h

#include <stdint.h>
#include <va/va.h>

void *vaapiMapBuffer(VADisplay dpy, VABufferID bufId);

void vaapiUnmapBuffer(VADisplay dpy, VABufferID bufId, void **pbuf);

bool
vaapiCreateBuffer(VADisplay dpy,
                  VAContextID ctx,
                  int type,
                  unsigned int size,
                  const void *data, VABufferID * bufId, void **mappedData);

void vaapiDestroyBuffer(VADisplay dpy, VABufferID * bufId);

const char *stringOfVAProfile(VAProfile profile);

const char *stringOfVAEntrypoint(VAEntrypoint entrypoint);

uint32_t fromVaapiSurfaceRenderFlags(uint32_t flags);

uint32_t toVaapiSurfaceStatus(uint32_t vaFlags);

uint32_t fromVaapiRotation(uint32_t value);

uint32_t toVaapiRotation(uint32_t value);

#endif                          /* vaapiutils_h */
