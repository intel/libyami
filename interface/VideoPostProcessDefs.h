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
} VppParamType;

typedef struct VppParamOsd {
    size_t size;
    uint32_t threshold;
} VppParamOsd;

#ifdef __cplusplus
}
#endif
#endif /*  __VIDEO_POST_PROCESS_DEFS_H__ */
