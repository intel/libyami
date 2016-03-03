/*
 *  vaapiutils.c - VA-API utilities
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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "vaapiutils.h"
#include "common/log.h"
#include "vaapisurface.h"
#include <stdarg.h>
#include <stdio.h>

#define CONCAT(a, b)    CONCAT_(a, b)
#define CONCAT_(a, b)   a##b
#define STRINGIFY(x)    STRINGIFY_(x)
#define STRINGIFY_(x)   #x
#define STRCASEP(p, x)  STRCASE(CONCAT(p, x))
#define STRCASE(x)      case x: return STRINGIFY(x)

namespace YamiMediaCodec{
/* Maps VA buffer */
void *vaapiMapBuffer(VADisplay dpy, VABufferID bufId)
{
    VAStatus status;
    void *data = NULL;

    status = vaMapBuffer(dpy, bufId, &data);

    if (!checkVaapiStatus(status, "vaMapBuffer()")) {
        ERROR("fail to map bufId = %x", bufId);
        return NULL;
    }

    return data;
}

/* Unmaps VA buffer */
void vaapiUnmapBuffer(VADisplay dpy, VABufferID bufId, void **pbuf)
{
    VAStatus status;

    if (pbuf)
        *pbuf = NULL;

    status = vaUnmapBuffer(dpy, bufId);
    if (!checkVaapiStatus(status, "vaUnmapBuffer()"))
        return;
}

/* Creates and maps VA buffer */
bool
vaapiCreateBuffer(VADisplay dpy,
                  VAContextID ctx,
                  int type,
                  uint32_t size,
                  const void *buf,
                  VABufferID * bufIdPtr, void **mappedData)
{
    VABufferID bufId;
    VAStatus status;
    void *data = (void *) buf;

    status =
        vaCreateBuffer(dpy, ctx, (VABufferType) type, size, 1, data,
                       &bufId);
    if (!checkVaapiStatus(status, "vaCreateBuffer()"))
        return false;

    if (mappedData) {
        data = vaapiMapBuffer(dpy, bufId);
        if (!data)
            goto error;
        *mappedData = data;
    }

    *bufIdPtr = bufId;
    return true;

  error:
    vaapiDestroyBuffer(dpy, &bufId);
    return false;
}

/* Destroy VA buffer */
void vaapiDestroyBuffer(VADisplay dpy, VABufferID * bufIdPtr)
{
    if (!bufIdPtr || *bufIdPtr == VA_INVALID_ID)
        return;

    vaDestroyBuffer(dpy, *bufIdPtr);
    *bufIdPtr = VA_INVALID_ID;
}

}
