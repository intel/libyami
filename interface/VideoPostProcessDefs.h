/*
 * Copyright (C) 2016 Intel Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __VIDEO_POST_PROCESS_DEFS_H__
#define __VIDEO_POST_PROCESS_DEFS_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VppParamTypeOsd,
    VppParamTypeTransform,
    VppParamTypeMosaic,
    VppParamTypeDenoise,
    VppParamTypeSharpening,
    VppParamTypeDeinterlace,
    VppParamTypeWireframe,
    VppParamTypeColorBalance
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

typedef struct VppParamMosaic {
    size_t size;
    uint32_t blockSize;
} VppParamMosaic;

#define DENOISE_LEVEL_MIN 0
#define DENOISE_LEVEL_MAX 100
#define DENOISE_LEVEL_NONE (DENOISE_LEVEL_MIN - 1)

typedef struct VPPDenoiseParameters {
    size_t size;
    int32_t level;
} VPPDenoiseParameters;

#define SHARPENING_LEVEL_MIN 0
#define SHARPENING_LEVEL_MAX 100
#define SHARPENING_LEVEL_NONE (SHARPENING_LEVEL_MIN - 1)

#define COLORBALANCE_LEVEL_MIN 0
#define COLORBALANCE_LEVEL_MAX 100
#define COLORBALANCE_LEVEL_NONE (COLORBALANCE_LEVEL_MIN - 1)

typedef struct VPPSharpeningParameters {
    size_t size;
    int32_t level;
} VPPSharpeningParameters;

typedef enum VppDeinterlaceMode {
    DEINTERLACE_MODE_NONE,
    DEINTERLACE_MODE_WEAVE = DEINTERLACE_MODE_NONE,
    DEINTERLACE_MODE_BOB,
} VppDeinterlaceMode;

typedef struct VPPDeinterlaceParameters {
    size_t size;
    VppDeinterlaceMode mode;
} VPPDeinterlaceParameters;

typedef struct VppParamWireframe {
    size_t size;
    uint32_t borderWidth;
    uint8_t colorY;
    uint8_t colorU;
    uint8_t colorV;
} VppParamWireframe;

typedef enum VppColorBalanceMode {
    COLORBALANCE_NONE = 0,
    COLORBALANCE_HUE,
    COLORBALANCE_SATURATION,
    COLORBALANCE_BRIGHTNESS,
    COLORBALANCE_CONTRAST,
    COLORBALANCE_COUNT
} VppColorBalanceMode;

typedef struct VPPColorBalanceParameter {
    size_t size;
    VppColorBalanceMode mode;
    int32_t level;
} VPPColorBalanceParameter;

#ifdef __cplusplus
}
#endif
#endif /*  __VIDEO_POST_PROCESS_DEFS_H__ */
