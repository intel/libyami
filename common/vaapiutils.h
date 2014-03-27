/*
 *  vaapiutils.h - VA-API utilities
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

#ifndef _VAAPI_UTILS_H
#define _VAAPI_UTILS_H

#include <stdint.h>
#include <va/va.h>
#include "log.h"

bool vaapi_check_status(VAStatus status, const char *msg);

void *vaapi_map_buffer(VADisplay dpy, VABufferID buf_id);

void vaapi_unmap_buffer(VADisplay dpy, VABufferID buf_id, void **pbuf);

bool
vaapi_create_buffer(VADisplay dpy,
		    VAContextID ctx,
		    int type,
		    unsigned int size,
		    const void *data,
		    VABufferID * buf_id, void **mapped_data);

void vaapi_destroy_buffer(VADisplay dpy, VABufferID * buf_id);

const char *string_of_VAProfile(VAProfile profile);

const char *string_of_VAEntrypoint(VAEntrypoint entrypoint);

uint32_t from_VaapiSurfaceRenderFlags(uint32_t flags);

uint32_t to_VaapiSurfaceStatus(uint32_t va_flags);

uint32_t from_VaapiRotation(uint32_t value);

uint32_t to_VaapiRotation(uint32_t value);

#endif				/* _VAAPI_UTILS_H */
