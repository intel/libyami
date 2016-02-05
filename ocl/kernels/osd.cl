/*
 *  osd.cl - osd with contrastive font color
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
    UV = alpha0 * 0.5 + (1 - alpha0) * UV;

    write_imagef(img_y_dst,  (int2)(id_x, id_y), Y0);
    write_imagef(img_y_dst,  (int2)(id_x, id_y + 1), Y1);
    write_imagef(img_uv_dst, (int2)(id_x, id_y / 2), UV);
}
