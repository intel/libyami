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

__kernel void mosaic(__write_only image2d_t img_y_dst,
                     __read_only image2d_t img_y,
                     __write_only image2d_t img_uv_dst,
                     __read_only image2d_t img_uv,
                     uint crop_x,
                     uint crop_y,
                     uint crop_w,
                     uint crop_h,
                     uint blk_size,
                     __local float* sum)
{
    sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;
    size_t g_id_x = get_global_id(0);
    size_t g_id_y = get_global_id(1);
    size_t l_id_x = get_local_id(0);
    uint imax;

    float4 Y;
    sum[l_id_x] = 0;
    for (uint i = 0; i < blk_size; i++) {
        Y = read_imagef(img_y, (int2)(g_id_x + crop_x, g_id_y * blk_size + i + crop_y));
        sum[l_id_x] += Y.x;
    }
    barrier(CLK_LOCAL_MEM_FENCE);
    if (l_id_x % blk_size == 0) {
        for (uint i = 1; i < blk_size; i++) {
            sum[l_id_x] += sum[l_id_x + i];
        }
        sum[l_id_x] /= (blk_size * blk_size);
    }
    barrier(CLK_LOCAL_MEM_FENCE);
    Y.x = sum[l_id_x / blk_size * blk_size];
    imax = min(blk_size, crop_h - g_id_y * blk_size);
    if (g_id_x < crop_w) {
        for (uint i = 0; i < imax; i++) {
            write_imagef(img_y_dst, (int2)(g_id_x + crop_x, g_id_y * blk_size + i + crop_y), Y);
        }
    }

    float4 UV;
    sum[l_id_x] = 0;
    for (uint i = 0; i < blk_size / 2; i++) {
        UV = read_imagef(img_uv, (int2)(g_id_x + crop_x, g_id_y * blk_size / 2 + i + crop_y / 2));
        sum[l_id_x] += UV.x;
    }
    barrier(CLK_LOCAL_MEM_FENCE);
    if (l_id_x % blk_size == 0 || l_id_x % blk_size == 1) {
        for (uint i = 2; i < blk_size; i+=2) {
            sum[l_id_x] += sum[l_id_x + i];
        }
        sum[l_id_x] /= (blk_size * blk_size / 4);
    }
    barrier(CLK_LOCAL_MEM_FENCE);
    if (l_id_x % 2 == 0) {
        UV.x = sum[l_id_x / blk_size * blk_size];
    } else {
        UV.x = sum[l_id_x / blk_size * blk_size + 1];
    }
    imax /= 2;
    if (g_id_x < crop_w) {
        for (uint i = 0; i < blk_size / 2; i++) {
            write_imagef(img_uv_dst, (int2)(g_id_x + crop_x, g_id_y * blk_size / 2 + i + crop_y / 2), UV);
        }
    }
}
