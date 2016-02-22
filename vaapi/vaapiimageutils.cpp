/*
 *  vaapiimageutils.cpp - vaapi image utils
 *
 *  Copyright (C) 2011-2014 Intel Corporation
 *    Author: Xu Guangxin<guangxin.xu@intel.com>
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

#include "vaapiimageutils.h"

#include "vaapiutils.h"

namespace YamiMediaCodec{

uint8_t* mapSurfaceToImage(VADisplay display, intptr_t surface, VAImage& image)
{
    uint8_t* p = NULL;
    VAStatus status = vaDeriveImage(display, (VASurfaceID)surface, &image);
    if (!checkVaapiStatus(status, "vaDeriveImage"))
        return NULL;
    status = vaMapBuffer(display, image.buf, (void**)&p);
    if (!checkVaapiStatus(status, "vaMapBuffer")) {
        checkVaapiStatus(vaDestroyImage(display, image.image_id), "vaDestroyImage");
        return NULL;
    }
    return p;
}

void unmapImage(VADisplay display, const VAImage& image)
{
    checkVaapiStatus(vaUnmapBuffer(display, image.buf), "vaUnmapBuffer");
    checkVaapiStatus(vaDestroyImage(display, image.image_id), "vaDestroyImage");
}

}
