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

class VaapiImage
{
  private:
    typedef SharedPtr<VAImage> VAImagePtr;
    typedef WeakPtr<VaapiImageRaw> ImageRawWeakPtr;
    friend class VaapiImageRaw;
  public:
    static ImagePtr create(const DisplayPtr&,
               uint32_t format, uint32_t width, uint32_t height);
    static ImagePtr derive(const SurfacePtr&);
    static ImagePtr derive(const DisplayPtr&, const SurfacePtr&);
    ~VaapiImage();

    uint32_t getFormat();
    VAImageID getID();
    uint32_t getWidth();
    uint32_t getHeight();

    friend ImageRawPtr mapVaapiImage(ImagePtr, VideoDataMemoryType memoryType);

  private:
    VaapiImage(const DisplayPtr& display, const VAImagePtr& image);
    VaapiImage(const DisplayPtr& display, const SurfacePtr& surface, const VAImagePtr& image);

    DisplayPtr m_display;
    //hold a reference to surface
    //only use for derived image,
    SurfacePtr m_surface;
    VAImagePtr m_image;
    ImageRawWeakPtr m_rawImage;
    DISALLOW_COPY_AND_ASSIGN(VaapiImage);
};

//wired thing happend when we do two things together
// 1. make VaapiImage public shared_from_this (if you make map as VaapiImage's member, you do this)
// 2. alloc and set ImagePtr's deleter in VaapiImagePool. We also add ImagePoolPtr as Image's member.
// it will call deleter's operator(), but the deleter will never out of scope. So ImagePoolPtr will never get chance to be del.
// make map a standalone function can break 1.
ImageRawPtr mapVaapiImage(ImagePtr, VideoDataMemoryType memoryType = VIDEO_DATA_MEMORY_TYPE_RAW_COPY);

/* Raw image to store mapped image*/
class VaapiImageRaw {
private:
    typedef VaapiImage::VAImagePtr VAImagePtr;
    typedef VAStatus (*RealeaseCallback)(VADisplay, VABufferID);
public:
    static ImageRawPtr create(const DisplayPtr&, const ImagePtr&, VideoDataMemoryType);
    VideoDataMemoryType getMemoryType();
    bool copyTo(uint8_t* dest, const uint32_t offsets[3], const uint32_t pitches[3]);
    bool copyFrom(const uint8_t* src, const uint32_t offsets[3], const uint32_t pitches[3]);
    bool copyFrom(const uint8_t* src, uint32_t size);
    bool getHandle(intptr_t& handle, uint32_t offsets[3], uint32_t pitches[3]);
    ~VaapiImageRaw();
private:
    VaapiImageRaw(const DisplayPtr&, const ImagePtr&, VideoDataMemoryType,
                uintptr_t handle, RealeaseCallback);
    void getPlaneResolution(uint32_t width[3], uint32_t height[3], uint32_t& planes);
    bool copy(uint8_t* destBase,
              const uint32_t destOffsets[3], const uint32_t destPitches[3],
              const uint8_t* srcBase,
              const uint32_t srcOffsets[3], const uint32_t srcPitches[3]);
    static bool copy(uint8_t* destBase,
              const uint32_t destOffsets[3], const uint32_t destPitches[3],
              const uint8_t* srcBase,
              const uint32_t srcOffsets[3], const uint32_t srcPitches[3],
              const uint32_t width[3], const uint32_t height[3], uint32_t planes);

    DisplayPtr m_display;
    ImagePtr m_image;
    intptr_t m_handle;
    VideoDataMemoryType m_memoryType;
    RealeaseCallback m_release;

};

}
#endif                          /* VAAPI_IMAGE_H */
