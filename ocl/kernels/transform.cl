/*
 *  transform.cl - flip and rotate
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

__kernel void transform_flip_h(__write_only image2d_t img_y_dst,
                               __write_only image2d_t img_uv_dst,
                               __read_only image2d_t img_y_src,
                               __read_only image2d_t img_uv_src,
                               uint w)
{
    sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE |
                        CLK_ADDRESS_CLAMP_TO_EDGE   |
                        CLK_FILTER_NEAREST;
    size_t g_id_x = get_global_id(0);
    size_t g_id_y = get_global_id(1);
    float4 src, dst;

    int coord_y = 2 * g_id_y;
    src = read_imagef(img_y_src, sampler, (int2)(g_id_x, coord_y));
    dst = (float4)(src.w, src.z, src.y, src.x);
    write_imagef(img_y_dst, (int2)(w - g_id_x, coord_y), dst);

    src = read_imagef(img_y_src, sampler, (int2)(g_id_x, coord_y + 1));
    dst = (float4)(src.w, src.z, src.y, src.x);
    write_imagef(img_y_dst, (int2)(w - g_id_x, coord_y + 1), dst);

    src = read_imagef(img_uv_src, sampler, (int2)(g_id_x, g_id_y));
    dst = (float4)(src.z, src.w, src.x, src.y);
    write_imagef(img_uv_dst, (int2)(w - g_id_x, g_id_y), dst);
}

__kernel void transform_flip_v(__write_only image2d_t img_y_dst,
                               __write_only image2d_t img_uv_dst,
                               __read_only image2d_t img_y_src,
                               __read_only image2d_t img_uv_src,
                               uint h)
{
    sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE |
                        CLK_ADDRESS_CLAMP_TO_EDGE   |
                        CLK_FILTER_NEAREST;
    size_t g_id_x = get_global_id(0);
    size_t g_id_y = get_global_id(1);
    float4 data;

    int coord_y = 2 * g_id_y;
    data = read_imagef(img_y_src, sampler, (int2)(g_id_x, coord_y));
    write_imagef(img_y_dst, (int2)(g_id_x, h - 1 - coord_y), data);
    data = read_imagef(img_y_src, sampler, (int2)(g_id_x, coord_y + 1));
    write_imagef(img_y_dst, (int2)(g_id_x, h - 1 - coord_y - 1), data);

    data = read_imagef(img_uv_src, sampler, (int2)(g_id_x, g_id_y));
    write_imagef(img_uv_dst, (int2)(g_id_x, h / 2 - g_id_y), data);
}

__kernel void transform_rot_180(__write_only image2d_t img_y_dst,
                                __write_only image2d_t img_uv_dst,
                                __read_only image2d_t img_y_src,
                                __read_only image2d_t img_uv_src,
                                uint w,
                                uint h)
{
    sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE |
                        CLK_ADDRESS_CLAMP_TO_EDGE   |
                        CLK_FILTER_NEAREST;
    size_t g_id_x = get_global_id(0);
    size_t g_id_y = get_global_id(1);
    float4 src, dst;

    int coord_y = 2 * g_id_y;
    src = read_imagef(img_y_src, sampler, (int2)(g_id_x, coord_y));
    dst = (float4)(src.w, src.z, src.y, src.x);
    write_imagef(img_y_dst, (int2)(w - g_id_x, h - 1 - coord_y), dst);

    src = read_imagef(img_y_src, sampler, (int2)(g_id_x, coord_y + 1));
    dst = (float4)(src.w, src.z, src.y, src.x);
    write_imagef(img_y_dst, (int2)(w - g_id_x, h - 1 - coord_y - 1), dst);

    src = read_imagef(img_uv_src, sampler, (int2)(g_id_x, g_id_y));
    dst = (float4)(src.z, src.w, src.x, src.y);
    write_imagef(img_uv_dst, (int2)(w - g_id_x, h / 2 - g_id_y), dst);
}

__kernel void transform_rot_90(__write_only image2d_t img_y_dst,
                               __write_only image2d_t img_uv_dst,
                               __read_only image2d_t img_y_src,
                               __read_only image2d_t img_uv_src,
                               uint w,
                               uint h)
{
    sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE |
                        CLK_ADDRESS_CLAMP_TO_EDGE   |
                        CLK_FILTER_NEAREST;
    size_t g_id_x = get_global_id(0);
    size_t g_id_y = get_global_id(1);
    float4 src0, src1, src2, src3, dst;

    int coord_x = 4 * g_id_x;
    int coord_y = 4 * g_id_y;
    src0 = read_imagef(img_y_src, sampler, (int2)(g_id_x, coord_y));
    src1 = read_imagef(img_y_src, sampler, (int2)(g_id_x, coord_y + 1));
    src2 = read_imagef(img_y_src, sampler, (int2)(g_id_x, coord_y + 2));
    src3 = read_imagef(img_y_src, sampler, (int2)(g_id_x, coord_y + 3));
    dst = (float4)(src3.x, src2.x, src1.x, src0.x);
    write_imagef(img_y_dst, (int2)(h - g_id_y, coord_x), dst);
    dst = (float4)(src3.y, src2.y, src1.y, src0.y);
    write_imagef(img_y_dst, (int2)(h - g_id_y, coord_x + 1), dst);
    dst = (float4)(src3.z, src2.z, src1.z, src0.z);
    write_imagef(img_y_dst, (int2)(h - g_id_y, coord_x + 2), dst);
    dst = (float4)(src3.w, src2.w, src1.w, src0.w);
    write_imagef(img_y_dst, (int2)(h - g_id_y, coord_x + 3), dst);

    coord_x = 2 * g_id_x;
    coord_y = 2 * g_id_y;
    src0 = read_imagef(img_uv_src, sampler, (int2)(g_id_x, coord_y));
    src1 = read_imagef(img_uv_src, sampler, (int2)(g_id_x, coord_y + 1));
    dst = (float4)(src1.x, src1.y, src0.x, src0.y);
    write_imagef(img_uv_dst, (int2)(h - g_id_y, coord_x), dst);
    dst = (float4)(src1.z, src1.w, src0.z, src0.w);
    write_imagef(img_uv_dst, (int2)(h - g_id_y, coord_x + 1), dst);
}

__kernel void transform_rot_270(__write_only image2d_t img_y_dst,
                                __write_only image2d_t img_uv_dst,
                                __read_only image2d_t img_y_src,
                                __read_only image2d_t img_uv_src,
                                uint w,
                                uint h)
{
    sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE |
                        CLK_ADDRESS_CLAMP_TO_EDGE   |
                        CLK_FILTER_NEAREST;
    size_t g_id_x = get_global_id(0);
    size_t g_id_y = get_global_id(1);
    float4 src0, src1, src2, src3, dst;

    int coord_x = 4 * g_id_x;
    int coord_y = 4 * g_id_y;
    src0 = read_imagef(img_y_src, sampler, (int2)(g_id_x, coord_y));
    src1 = read_imagef(img_y_src, sampler, (int2)(g_id_x, coord_y + 1));
    src2 = read_imagef(img_y_src, sampler, (int2)(g_id_x, coord_y + 2));
    src3 = read_imagef(img_y_src, sampler, (int2)(g_id_x, coord_y + 3));
    dst = (float4)(src0.w, src1.w, src2.w, src3.w);
    write_imagef(img_y_dst, (int2)(g_id_y, w * 4 - coord_x), dst);
    dst = (float4)(src0.z, src1.z, src2.z, src3.z);
    write_imagef(img_y_dst, (int2)(g_id_y, w * 4 - coord_x + 1), dst);
    dst = (float4)(src0.y, src1.y, src2.y, src3.y);
    write_imagef(img_y_dst, (int2)(g_id_y, w * 4 - coord_x + 2), dst);
    dst = (float4)(src0.x, src1.x, src2.x, src3.x);
    write_imagef(img_y_dst, (int2)(g_id_y, w * 4 - coord_x + 3), dst);

    coord_x = 2 * g_id_x;
    coord_y = 2 * g_id_y;
    src0 = read_imagef(img_uv_src, sampler, (int2)(g_id_x, coord_y));
    src1 = read_imagef(img_uv_src, sampler, (int2)(g_id_x, coord_y + 1));
    dst = (float4)(src0.z, src0.w, src1.z, src1.w);
    write_imagef(img_uv_dst, (int2)(g_id_y, w * 2 - coord_x), dst);
    dst = (float4)(src0.x, src0.y, src1.x, src1.y);
    write_imagef(img_uv_dst, (int2)(g_id_y, w * 2 - coord_x + 1), dst);
}

__kernel void transform_flip_h_rot_90(__write_only image2d_t img_y_dst,
                                      __write_only image2d_t img_uv_dst,
                                      __read_only image2d_t img_y_src,
                                      __read_only image2d_t img_uv_src,
                                      uint w,
                                      uint h)
{
    sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE |
                        CLK_ADDRESS_CLAMP_TO_EDGE   |
                        CLK_FILTER_NEAREST;
    size_t g_id_x = get_global_id(0);
    size_t g_id_y = get_global_id(1);
    float4 src0, src1, src2, src3, dst;

    int coord_x = 4 * g_id_x;
    int coord_y = 4 * g_id_y;
    src0 = read_imagef(img_y_src, sampler, (int2)(g_id_x, coord_y));
    src1 = read_imagef(img_y_src, sampler, (int2)(g_id_x, coord_y + 1));
    src2 = read_imagef(img_y_src, sampler, (int2)(g_id_x, coord_y + 2));
    src3 = read_imagef(img_y_src, sampler, (int2)(g_id_x, coord_y + 3));
    dst = (float4)(src3.w, src2.w, src1.w, src0.w);
    write_imagef(img_y_dst, (int2)(h - g_id_y, w * 4 - coord_x), dst);
    dst = (float4)(src3.z, src2.z, src1.z, src0.z);
    write_imagef(img_y_dst, (int2)(h - g_id_y, w * 4 - coord_x + 1), dst);
    dst = (float4)(src3.y, src2.y, src1.y, src0.y);
    write_imagef(img_y_dst, (int2)(h - g_id_y, w * 4 - coord_x + 2), dst);
    dst = (float4)(src3.x, src2.x, src1.x, src0.x);
    write_imagef(img_y_dst, (int2)(h - g_id_y, w * 4 - coord_x + 3), dst);

    coord_x = 2 * g_id_x;
    coord_y = 2 * g_id_y;
    src0 = read_imagef(img_uv_src, sampler, (int2)(g_id_x, coord_y));
    src1 = read_imagef(img_uv_src, sampler, (int2)(g_id_x, coord_y + 1));
    dst = (float4)(src1.z, src1.w, src0.z, src0.w);
    write_imagef(img_uv_dst, (int2)(h - g_id_y, w * 2 - coord_x), dst);
    dst = (float4)(src1.x, src1.y, src0.x, src0.y);
    write_imagef(img_uv_dst, (int2)(h - g_id_y, w * 2 - coord_x + 1), dst);
}

__kernel void transform_flip_v_rot_90(__write_only image2d_t img_y_dst,
                                      __write_only image2d_t img_uv_dst,
                                      __read_only image2d_t img_y_src,
                                      __read_only image2d_t img_uv_src,
                                      uint w,
                                      uint h)
{
    sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE |
                        CLK_ADDRESS_CLAMP_TO_EDGE   |
                        CLK_FILTER_NEAREST;
    size_t g_id_x = get_global_id(0);
    size_t g_id_y = get_global_id(1);
    float4 src0, src1, src2, src3, dst;

    int coord_x = 4 * g_id_x;
    int coord_y = 4 * g_id_y;
    src0 = read_imagef(img_y_src, sampler, (int2)(g_id_x, coord_y));
    src1 = read_imagef(img_y_src, sampler, (int2)(g_id_x, coord_y + 1));
    src2 = read_imagef(img_y_src, sampler, (int2)(g_id_x, coord_y + 2));
    src3 = read_imagef(img_y_src, sampler, (int2)(g_id_x, coord_y + 3));
    dst = (float4)(src0.x, src1.x, src2.x, src3.x);
    write_imagef(img_y_dst, (int2)(g_id_y, coord_x), dst);
    dst = (float4)(src0.y, src1.y, src2.y, src3.y);
    write_imagef(img_y_dst, (int2)(g_id_y, coord_x + 1), dst);
    dst = (float4)(src0.z, src1.z, src2.z, src3.z);
    write_imagef(img_y_dst, (int2)(g_id_y, coord_x + 2), dst);
    dst = (float4)(src0.w, src1.w, src2.w, src3.w);
    write_imagef(img_y_dst, (int2)(g_id_y, coord_x + 3), dst);

    coord_x = 2 * g_id_x;
    coord_y = 2 * g_id_y;
    src0 = read_imagef(img_uv_src, sampler, (int2)(g_id_x, coord_y));
    src1 = read_imagef(img_uv_src, sampler, (int2)(g_id_x, coord_y + 1));
    dst = (float4)(src0.x, src0.y, src1.x, src1.y);
    write_imagef(img_uv_dst, (int2)(g_id_y, coord_x), dst);
    dst = (float4)(src0.z, src0.w, src1.z, src1.w);
    write_imagef(img_uv_dst, (int2)(g_id_y, coord_x + 1), dst);
}
