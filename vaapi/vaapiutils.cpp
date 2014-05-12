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
#include "log.h"
#include "vaapisurface.h"
#include <stdarg.h>
#include <stdio.h>

#define CONCAT(a, b)    CONCAT_(a, b)
#define CONCAT_(a, b)   a##b
#define STRINGIFY(x)    STRINGIFY_(x)
#define STRINGIFY_(x)   #x
#define STRCASEP(p, x)  STRCASE(CONCAT(p, x))
#define STRCASE(x)      case x: return STRINGIFY(x)

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

/* Return a string representation of a VAProfile */
const char *stringOfVAProfile(VAProfile profile)
{
    switch (profile) {
#define MAP(profile) \
        STRCASEP(VAProfile, profile)
        MAP(MPEG2Simple);
        MAP(MPEG2Main);
        MAP(MPEG4Simple);
        MAP(MPEG4AdvancedSimple);
        MAP(MPEG4Main);
#if VA_CHECK_VERSION(0,32,0)
        MAP(JPEGBaseline);
        MAP(H263Baseline);
        MAP(H264ConstrainedBaseline);
#endif
        MAP(H264Baseline);
        MAP(H264Main);
        MAP(H264High);
        MAP(VC1Simple);
        MAP(VC1Main);
        MAP(VC1Advanced);
#undef MAP
    default:
        break;
    }
    return "<unknown>";
}

/* Return a string representation of a VAEntrypoint */
const char *stringOfVAEntrypoint(VAEntrypoint entrypoint)
{
    switch (entrypoint) {
#define MAP(entrypoint) \
        STRCASEP(VAEntrypoint, entrypoint)
        MAP(VLD);
        MAP(IZZ);
        MAP(IDCT);
        MAP(MoComp);
        MAP(Deblocking);
#undef MAP
    default:
        break;
    }
    return "<unknown>";
}

#if 0
/**
 * fromVaapiSurfaceRenderFlags:
 * @flags: the #VaapiSurfaceRenderFlags
 *
 * Converts #VaapiSurfaceRenderFlags to flags suitable for
 * vaPutSurface().
 */
uint32_t fromVaapiSurfaceRenderFlags(uint32_t flags)
{
    uint32_t va_fields = 0, va_csc = 0;

    if (flags & VAAPI_PICTURE_STRUCTURE_TOP_FIELD)
        va_fields |= VA_TOP_FIELD;
    if (flags & VAAPI_PICTURE_STRUCTURE_BOTTOM_FIELD)
        va_fields |= VA_BOTTOM_FIELD;
    if ((va_fields ^ (VA_TOP_FIELD | VA_BOTTOM_FIELD)) == 0)
        va_fields = VA_FRAME_PICTURE;

#ifdef VA_SRC_BT601
    if (flags & VAAPI_COLOR_STANDARD_ITUR_BT_601)
        va_csc = VA_SRC_BT601;
#endif
#ifdef VA_SRC_BT709
    if (flags & VAAPI_COLOR_STANDARD_ITUR_BT_709)
        va_csc = VA_SRC_BT709;
#endif

    return va_fields | va_csc;
}
#endif

/**
 * toVaapiSurfaceStatus:
 * @flags: the #VaapiSurfaceStatus flags to translate
 *
 * Converts vaQuerySurfaceStatus() @flags to #VaapiSurfaceStatus
 * flags.
 *
 * Return value: the #VaapiSurfaceStatus flags
 */
uint32_t toVaapiSurfaceStatus(uint32_t vaFlags)
{
    uint32_t flags;
    const uint32_t vaFlagsMask = (VASurfaceReady |
                                  VASurfaceRendering |
                                  VASurfaceDisplaying);

    /* Check for core status */
    switch (vaFlags & vaFlagsMask) {
    case VASurfaceReady:
        flags = VAAPI_SURFACE_STATUS_IDLE;
        break;
    case VASurfaceRendering:
        flags = VAAPI_SURFACE_STATUS_RENDERING;
        break;
    case VASurfaceDisplaying:
        flags = VAAPI_SURFACE_STATUS_DISPLAYING;
        break;
    default:
        flags = 0;
        break;
    }

    /* Check for encoder status */
#if VA_CHECK_VERSION(0,30,0)
    if (vaFlags & VASurfaceSkipped)
        flags |= VAAPI_SURFACE_STATUS_SKIPPED;
#endif
    return flags;
}

/* Translate VaapiRotation value to VA-API rotation value */
uint32_t fromVaapiRotation(uint32_t value)
{
    switch (value) {
    case VAAPI_ROTATION_0:
        return VA_ROTATION_NONE;
    case VAAPI_ROTATION_90:
        return VA_ROTATION_90;
    case VAAPI_ROTATION_180:
        return VA_ROTATION_180;
    case VAAPI_ROTATION_270:
        return VA_ROTATION_270;
    }
    ERROR("unsupported VaapiRotation value %d", value);
    return VA_ROTATION_NONE;
}

/* Translate VA-API rotation value to VaapiRotation value */
uint32_t toVaapiRotation(uint32_t value)
{
    switch (value) {
    case VA_ROTATION_NONE:
        return VAAPI_ROTATION_0;
    case VA_ROTATION_90:
        return VAAPI_ROTATION_90;
    case VA_ROTATION_180:
        return VAAPI_ROTATION_180;
    case VA_ROTATION_270:
        return VAAPI_ROTATION_270;
    }
    ERROR("unsupported VA-API rotation value %d", value);
    return VAAPI_ROTATION_0;
}
