/*
 *  VideoPostProcessDefs.h - video postprocessing definitions
 *
 *  Copyright (C) 2016 Intel Corporation
 *    Author: Jia Meng<jia.meng@intel.com>
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

#ifndef __VIDEO_POST_PROCESS_DEFS_H__
#define __VIDEO_POST_PROCESS_DEFS_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VppParamTypeOsd,
    VppParamTypeTransform,
} VppParamType;

typedef struct VppParamOsd {
    size_t size;
    uint32_t threshold;
} VppParamOsd;

typedef enum {
    VPP_TRANSFORM_NONE   = 0x0,
    VPP_TRANSFORM_FLIP_H = 0x1,
    VPP_TRANSFORM_FLIP_V = 0x2,
    VPP_TRANSFORM_ROT_90 = 0x4,
    VPP_TRANSFORM_ROT_180 = 0x8,
    VPP_TRANSFORM_ROT_270 = 0x10,
} VppTransform;

typedef struct VppParamTransform {
    size_t size;
    uint32_t transform;
} VppParamTransform;

#ifdef __cplusplus
}
#endif
#endif /*  __VIDEO_POST_PROCESS_DEFS_H__ */
