/*
 * Copyright (C) 2015 Intel Corporation. All rights reserved.
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

__kernel void blend(__write_only image2d_t dst_y,
                    __write_only image2d_t dst_uv,
                    __read_only image2d_t bg_y,
                    __read_only image2d_t bg_uv,
                    __read_only image2d_t fg,
                    uint crop_x, uint crop_y, uint crop_w, uint crop_h)
{
    sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;
    int i;
    int id_x = get_global_id(0);
    int id_y = get_global_id(1) * 2;
    int id_z = id_x;
    int id_w = id_y;

    float4 y1, y2;
    float4 y1_dst, y2_dst, y_dst;
    float4 uv, uv_dst;
    float4 rgba[4];

    id_x += crop_x;
    id_y += crop_y;
    y1 = read_imagef(bg_y, sampler, (int2)(id_x, id_y));
    y2 = read_imagef(bg_y, sampler, (int2)(id_x, id_y + 1));
    uv = read_imagef(bg_uv, sampler, (int2)(id_x, id_y / 2));

    rgba[0] = read_imagef(fg, sampler, (int2)(2 * id_z,     id_w));
    rgba[1] = read_imagef(fg, sampler, (int2)(2 * id_z + 1, id_w));
    rgba[2] = read_imagef(fg, sampler, (int2)(2 * id_z    , id_w + 1));
    rgba[3] = read_imagef(fg, sampler, (int2)(2 * id_z + 1, id_w + 1));

    y_dst = 0.299f * (float4) (rgba[0].x, rgba[1].x, rgba[2].x, rgba[3].x);
    y_dst = mad(0.587f, (float4) (rgba[0].y, rgba[1].y, rgba[2].y, rgba[3].y), y_dst);
    y_dst = mad(0.114f, (float4) (rgba[0].z, rgba[1].z, rgba[2].z, rgba[3].z), y_dst);
    y_dst *= (float4) (rgba[0].w, rgba[1].w, rgba[2].w, rgba[3].w);
    y1_dst.x = mad(1 - rgba[0].w, y1.x, y_dst.x);
    y1_dst.y = mad(1 - rgba[1].w, y1.y, y_dst.y);
    y2_dst.x = mad(1 - rgba[2].w, y2.x, y_dst.z);
    y2_dst.y = mad(1 - rgba[3].w, y2.y, y_dst.w);

    uv_dst.x = rgba[0].w * (-0.14713f * rgba[0].x - 0.28886f * rgba[0].y + 0.43600f * rgba[0].z + 0.5f);
    uv_dst.y = rgba[0].w * ( 0.61500f * rgba[0].x - 0.51499f * rgba[0].y - 0.10001f * rgba[0].z + 0.5f);
    uv_dst.x = mad(1 - rgba[0].w, uv.x, uv_dst.x);
    uv_dst.y = mad(1 - rgba[0].w, uv.y, uv_dst.y);

    if (id_z <= crop_w && id_w <= crop_h) {
        write_imagef(dst_y, (int2)(id_x, id_y), y1_dst);
        write_imagef(dst_y, (int2)(id_x, id_y + 1), y2_dst);
        write_imagef(dst_uv, (int2)(id_x, id_y / 2), uv_dst);
    }
}
