/*
 *  vaapisurface.h - VA surface abstraction
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

#ifndef vaapisurface_h
#define vaapisurface_h

#include "vaapiimage.h"
#include "vaapiptrs.h"
#include "vaapitypes.h"
#include <va/va_drmcommon.h>
#include <va/va.h>
#include "common/common_def.h"

namespace YamiMediaCodec{
typedef enum {
    VAAPI_CHROMA_TYPE_YUV400,
    VAAPI_CHROMA_TYPE_YUV420,
    VAAPI_CHROMA_TYPE_YUV422,
    VAAPI_CHROMA_TYPE_YUV444
} VaapiChromaType;

typedef enum {
    VAAPI_SURFACE_STATUS_IDLE = 1 << 0,
    VAAPI_SURFACE_STATUS_RENDERING = 1 << 1,
    VAAPI_SURFACE_STATUS_DISPLAYING = 1 << 2,
    VAAPI_SURFACE_STATUS_SKIPPED = 1 << 3
} VaapiSurfaceStatus;

class VaapiSurface
{
  private:
    DISALLOW_COPY_AND_ASSIGN(VaapiSurface);
  public:
    static SurfacePtr create(const DisplayPtr&,
                             VaapiChromaType,
                             uint32_t width,
                             uint32_t height,
                             void *surfaceAttribArray,
                             uint32_t surfAttribNum);
    VaapiSurface(const DisplayPtr&, VASurfaceID);
    VaapiSurface(intptr_t id, uint32_t width, uint32_t height);

    ~VaapiSurface();

    VaapiChromaType getChromaType(void);
    VASurfaceID getID(void) const;
    uint32_t getWidth(void);
    uint32_t getHeight(void);
    bool resize(uint32_t width, uint32_t height);
    DisplayPtr getDisplay();

    bool sync();
    bool queryStatus(VaapiSurfaceStatus * pStatus);

    bool getImage(ImagePtr image);
    bool putImage(ImagePtr image);

  private:
    VaapiSurface(const DisplayPtr&,
                 VASurfaceID,
                 VaapiChromaType,
                 uint32_t width,
                 uint32_t height);

    uint32_t toVaapiSurfaceStatus(uint32_t vaFlags);

    DisplayPtr m_display;
    VaapiChromaType m_chromaType;
    uint32_t m_allocWidth;
    uint32_t m_allocHeight;
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_fourcc;
    VASurfaceID m_ID;
    bool     m_owner;       //surface owner
};
}
#endif                          /* VAAPI_SURFACE_H */
