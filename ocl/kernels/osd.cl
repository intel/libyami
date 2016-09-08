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

#define PIXEL_SIZE 2
#define CELL_WIDTH 2
#define CELL_HEIGHT 2

__kernel void reduce_luma(__read_only image2d_t img_y,
                          int crop_x,
                          int crop_y,
                          int crop_h,
                          __global uint* acc)
{
    sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;
    size_t g_id = crop_x + get_global_id(0);
    uint4 Y = (uint4)(0, 0, 0, 0);

    int i;
    for (i = 0; i < crop_h; i++) {
        Y += read_imageui(img_y, sampler, (int2)(g_id, crop_y + i));
    }

    *((__global uint4 *)acc + get_global_id(0)) = Y;
}

__kernel void osd(__write_only image2d_t img_y_dst,
                  __write_only image2d_t img_uv_dst,
                  __read_only image2d_t img_y,
                  __read_only image2d_t img_uv,
                  __read_only image2d_t img_a,
                  int crop_x,
                  int crop_y,
                  int crop_w,
                  int crop_h,
                  __constant float* luma)
{
    sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;
    size_t g_id_x = get_global_id(0);
    size_t g_id_y = get_global_id(1);
    size_t l_id_x = get_local_id(0);
    size_t l_id_y = get_local_id(1);

    float4 Y0, Y1;
    float4 UV;
    float4 rgba, alpha0, alpha1;

    float bw = luma[g_id_x * PIXEL_SIZE / crop_h];

    int id_x = g_id_x * CELL_WIDTH;
    int id_y = g_id_y * CELL_HEIGHT;
    rgba = read_imagef(img_a, sampler, (int2)(id_x,     id_y));
    alpha0.x = rgba.w;
    rgba = read_imagef(img_a, sampler, (int2)(id_x + 1, id_y));
    alpha0.y = rgba.w;
    rgba = read_imagef(img_a, sampler, (int2)(id_x,     id_y + 1));
    alpha1.x = rgba.w;
    rgba = read_imagef(img_a, sampler, (int2)(id_x + 1, id_y + 1));
    alpha1.y = rgba.w;

    id_x = g_id_x + crop_x;
    id_y = g_id_y * CELL_HEIGHT + crop_y;
    Y0 = read_imagef(img_y_dst, sampler,  (int2)(id_x, id_y));
    Y1 = read_imagef(img_y_dst, sampler,  (int2)(id_x, id_y + 1));
    UV = read_imagef(img_uv_dst, sampler, (int2)(id_x, id_y / 2));

    // blend BW (1.0/0.0, 0.5, 0.5)
    Y0 = alpha0 * bw  + (1 - alpha0) * Y0;
    Y1 = alpha1 * bw  + (1 - alpha1) * Y1;
    UV = alpha0 * 0.5f + (1 - alpha0) * UV;

    write_imagef(img_y_dst,  (int2)(id_x, id_y), Y0);
    write_imagef(img_y_dst,  (int2)(id_x, id_y + 1), Y1);
    write_imagef(img_uv_dst, (int2)(id_x, id_y / 2), UV);
}
