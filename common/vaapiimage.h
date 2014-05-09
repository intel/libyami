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
#include <stdint.h>

/* Image format definition */
typedef enum {
    VAAPI_IMAGE_FORMAT_TYPE_YCBCR = 1,  /* YUV */
    VAAPI_IMAGE_FORMAT_TYPE_RGB,    /* RGB */
    VAAPI_IMAGE_FORMAT_TYPE_INDEXED /* paletted */
} VaapiImageFormatType;

typedef enum {
    VAAPI_IMAGE_NV12 = MAKE_FOURCC('N', 'V', '1', '2'),
    VAAPI_IMAGE_YV12 = MAKE_FOURCC('Y', 'V', '1', '2'),
    VAAPI_IMAGE_I420 = MAKE_FOURCC('I', '4', '2', '0'),
    VAAPI_IMAGE_UYVY = MAKE_FOURCC('U', 'Y', 'V', 'Y'),
    VAAPI_IMAGE_YUY2 = MAKE_FOURCC('Y', 'U', 'Y', '2'),
    VAAPI_IMAGE_AYUV = MAKE_FOURCC('A', 'Y', 'U', 'V'),
    VAAPI_IMAGE_ARGB = MAKE_FOURCC('A', 'R', 'G', 'B'),
    VAAPI_IMAGE_RGBA = MAKE_FOURCC('R', 'G', 'B', 'A'),
    VAAPI_IMAGE_ABGR = MAKE_FOURCC('A', 'B', 'G', 'R'),
    VAAPI_IMAGE_BGRA = MAKE_FOURCC('B', 'G', 'R', 'A')
} VaapiImageFormat;

typedef struct _VaapiImageFormatMap {
    VaapiImageFormatType type;
    VaapiImageFormat format;
    VAImageFormat vaFormat;
} VaapiImageFormatMap;

#define DEF(TYPE, FORMAT)                                              \
    VAAPI_IMAGE_FORMAT_TYPE_##TYPE,                                    \
    VAAPI_IMAGE_##FORMAT

#define DEF_YUV(FORMAT, FOURCC, ENDIAN, BPP)                           \
    { DEF(YCBCR, FORMAT),                                              \
      { VA_FOURCC FOURCC, VA_##ENDIAN##_FIRST, BPP, 0, 0, 0, 0, 0}, }

#define DEF_RGB(FORMAT, FOURCC, ENDIAN, BPP, DEPTH, R,G,B,A)           \
    { DEF(RGB, FORMAT),                                                \
      { VA_FOURCC FOURCC, VA_##ENDIAN##_FIRST, BPP, DEPTH, R,G,B,A }, }

const VaapiImageFormatMap vaapiImageFormats[] = {
    DEF_YUV(NV12, ('N', 'V', '1', '2'), LSB, 12),
    DEF_YUV(YV12, ('Y', 'V', '1', '2'), LSB, 12),
    DEF_YUV(I420, ('I', '4', '2', '0'), LSB, 12),
    DEF_YUV(AYUV, ('A', 'Y', 'U', 'V'), LSB, 32),
#if __BIG_ENDIAN__
    DEF_RGB(ARGB, ('A', 'R', 'G', 'B'), MSB, 32,
            32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000),
    DEF_RGB(ABGR, ('A', 'B', 'G', 'R'), MSB, 32,
            32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000),
#else
    DEF_RGB(BGRA, ('B', 'G', 'R', 'A'), LSB, 32,
            32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000),
    DEF_RGB(RGBA, ('R', 'G', 'B', 'A'), LSB, 32,
            32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000),
#endif
};

#undef DEF_RGB
#undef DEF_YUV
#undef DEF

/* Raw image to store mapped image*/
typedef struct _VaapiImageRaw {
    VaapiImageFormat format;
    uint32_t width;
    uint32_t height;
    uint32_t numPlanes;
    uint8_t *pixels[3];
    uint32_t strides[3];
    uint32_t size;
} VaapiImageRaw;

class VaapiImage {
  private:
    DISALLOW_COPY_AND_ASSIGN(VaapiImage);
  public:
    VaapiImage(VADisplay display,
               VaapiImageFormat format, uint32_t width, uint32_t height);

    VaapiImage(VADisplay display, VAImage * image);

    ~VaapiImage();

    VaapiImageFormat getFormat();
    VAImageID getID();
    uint32_t getWidth();
    uint32_t getHeight();

    VaapiImageRaw *map();
    bool isMapped();
    bool unmap();

  private:
    const VAImageFormat *getVaFormat(VaapiImageFormat format);

  private:
     VADisplay m_display;
    VaapiImageFormat m_format;
    VAImageID m_ID;
    uint32_t m_width;
    uint32_t m_height;

    VAImage m_image;
    uint8_t *m_data;
    bool m_isMapped;
    VaapiImageRaw m_rawImage;
};

#endif                          /* VAAPI_IMAGE_H */
