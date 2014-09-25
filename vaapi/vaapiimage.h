/*
 *  vaapiimage.h - VA image abstraction
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

#ifndef vaapiimage_h
#define vaapiimage_h

#include "vaapitypes.h"
#include <va/va.h>
#include <va/va_drmcommon.h>
#include <stdint.h>
#include "common/common_def.h"
#include "interface/VideoCommonDefs.h"
#include "vaapi/vaapiptrs.h"
#include "vaapi/vaapidisplay.h"

namespace YamiMediaCodec{
/* Raw image to store mapped image*/
typedef struct {
    VideoDataMemoryType memoryType;
    uint32_t format;
    uint32_t width;
    uint32_t height;
    uint32_t numPlanes;
    intptr_t handles[3];
    uint32_t strides[3];
    uint32_t size;
} VaapiImageRaw;

class VaapiImage {
  private:
    typedef std::tr1::shared_ptr<VAImage> VAImagePtr;
  public:
    static ImagePtr create(const DisplayPtr&,
               uint32_t format, uint32_t width, uint32_t height);
    static ImagePtr derive(const SurfacePtr&);
    ~VaapiImage();

    uint32_t getFormat();
    VAImageID getID();
    uint32_t getWidth();
    uint32_t getHeight();

    VaapiImageRaw *map(VideoDataMemoryType memoryType = VIDEO_DATA_MEMORY_TYPE_RAW_POINTER); //RAW_POINTER & RAW_COPY are same inside VaaiImage
    bool isMapped();
    bool unmap();

  private:
    VaapiImage(const DisplayPtr& display, const VAImagePtr& image);
    VaapiImage(const DisplayPtr& display, const SurfacePtr& surface, const VAImagePtr& image);

    DisplayPtr m_display;
    //hold a reference to surface
    //only use for derived image,
    SurfacePtr m_surface;
    VAImagePtr m_image;
    bool m_isMapped;
    VaapiImageRaw m_rawImage;
    DISALLOW_COPY_AND_ASSIGN(VaapiImage);
};
}
#endif                          /* VAAPI_IMAGE_H */
