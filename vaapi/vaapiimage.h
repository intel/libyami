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
    DISALLOW_COPY_AND_ASSIGN(VaapiImage);
  public:
    static ImagePtr create(DisplayPtr display,
               uint32_t format, uint32_t width, uint32_t height);
    static ImagePtr create(DisplayPtr display, VAImage * image);
    ~VaapiImage();

    uint32_t getFormat();
    VAImageID getID();
    uint32_t getWidth();
    uint32_t getHeight();

    VaapiImageRaw *map(VideoDataMemoryType memoryType = VIDEO_DATA_MEMORY_TYPE_RAW_POINTER); //RAW_POINTER & RAW_COPY are same inside VaaiImage
    bool isMapped();
    bool unmap();

  private:
    VaapiImage(DisplayPtr display,
                 uint32_t format, uint32_t width, uint32_t height);
    VaapiImage(DisplayPtr display, VAImage * image);

    DisplayPtr m_display;
    VAImage m_image;
    uint8_t *m_data;
    bool m_isMapped;
    VaapiImageRaw m_rawImage;
};
}
#endif                          /* VAAPI_IMAGE_H */
