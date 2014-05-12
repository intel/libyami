/*
 *  vaapitypes.h - Basic types
 *
 *  Copyright (C) 2010-2011 Splitted-Desktop Systems
 *    Author: Gwenole Beauchesne <gwenole.beauchesne@splitted-desktop.com>
 *  Copyright (C) 2012-2014 Intel Corporation
 *    Author: Gwenole Beauchesne <gwenole.beauchesne@intel.com>
 *    Author: Xiaowei Li <xiaowei.li@intel.com>
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

#ifndef vaapitypes_h
#define vaapitypes_h
#include <stdint.h>
#include <common_def.h>

typedef struct _VaapiPoint {
    uint32_t x;
    uint32_t y;
} VaapiPoint;

typedef struct _VaapiRectangle {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
} VaapiRectangle;

typedef enum {
    VAAPI_RENDER_MODE_OVERLAY = 1,
    VAAPI_RENDER_MODE_TEXTURE
} VaapiRenderMode;

typedef enum {
    VAAPI_ROTATION_0 = 0,
    VAAPI_ROTATION_90 = 90,
    VAAPI_ROTATION_180 = 180,
    VAAPI_ROTATION_270 = 270,
} VaapiRotation;

typedef enum {
    VAAPI_PROFILE_UNKNOWN = 0,
    VAAPI_PROFILE_MPEG1,
    VAAPI_PROFILE_MPEG2_SIMPLE,
    VAAPI_PROFILE_MPEG2_MAIN,
    VAAPI_PROFILE_MPEG2_HIGH,
    VAAPI_PROFILE_MPEG4_SIMPLE,
    VAAPI_PROFILE_MPEG4_ADVANCED_SIMPLE,
    VAAPI_PROFILE_MPEG4_MAIN,
    VAAPI_PROFILE_H263_BASELINE,
    VAAPI_PROFILE_H264_BASELINE,
    VAAPI_PROFILE_H264_CONSTRAINED_BASELINE,
    VAAPI_PROFILE_H264_MAIN,
    VAAPI_PROFILE_H264_HIGH,
    VAAPI_PROFILE_H264_HIGH10,
    VAAPI_PROFILE_H264_HIGH_422,
    VAAPI_PROFILE_H264_HIGH_444,
    VAAPI_PROFILE_H264_MULTIVIEW_HIGH,
    VAAPI_PROFILE_H264_STEREO_HIGH,
    VAAPI_PROFILE_VC1_SIMPLE,
    VAAPI_PROFILE_VC1_MAIN,
    VAAPI_PROFILE_VC1_ADVANCED,
    VAAPI_PROFILE_JPEG_BASELINE,
} VaapiProfile;

typedef enum {
    VAAPI_ENTRYPOINT_VLD = 1,
    VAAPI_ENTRYPOINT_IDCT,
    VAAPI_ENTRYPOINT_MOCO,
    VAAPI_ENTRYPOINT_SLICE_ENCODE
} VaapiEntrypoint;

#define MAKE_FOURCC(ch0, ch1, ch2, ch3) \
 (((unsigned long)(unsigned char) (ch0))      | ((unsigned long)(unsigned char) (ch1) << 8) | \
  ((unsigned long)(unsigned char) (ch2) << 16) | ((unsigned long)(unsigned char) (ch3) << 24 ))

#define DISALLOW_COPY_AND_ASSIGN(className) \
      className(const className&); \
      className & operator=(const className&); \


typedef enum {
    VAAPI_PICTURE_TYPE_NONE = 0,        // Undefined
    VAAPI_PICTURE_TYPE_I,               // Intra
    VAAPI_PICTURE_TYPE_P,               // Predicted
    VAAPI_PICTURE_TYPE_B,               // Bi-directional predicted
    VAAPI_PICTURE_TYPE_S,               // S(GMC)-VOP (MPEG-4)
    VAAPI_PICTURE_TYPE_SI,              // Switching Intra
    VAAPI_PICTURE_TYPE_SP,              // Switching Predicted
    VAAPI_PICTURE_TYPE_BI,              // BI type (VC-1)
} VaapiPictureType;


#endif                          /* vaapitypes_h */
