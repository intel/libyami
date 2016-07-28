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

__kernel void wireframe(__write_only image2d_t img_y,
                        __write_only image2d_t img_uv,
                        uint crop_x,
                        uint crop_y,
                        uint crop_w,
                        uint crop_h,
                        uint border,
                        uchar y,
                        uchar u,
                        uchar v)
{
    sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;
    size_t g_id_x = get_global_id(0);
    size_t g_id_y = get_global_id(1);
    uint4 Y = (uint4)y;
    uint4 UV = (uint4)(u, v, 0, 0);

    if ((g_id_x < crop_w && g_id_y < border)
       || ((g_id_x < border || (g_id_x + border >= crop_w && g_id_x < crop_w))
           && (g_id_y >= border && g_id_y + border < crop_h))
       || (g_id_x < crop_w && (g_id_y + border >= crop_h && g_id_y < crop_h))) {
        write_imageui(img_y, (int2)(g_id_x + crop_x, 2 * (g_id_y + crop_y)), Y);
        write_imageui(img_y, (int2)(g_id_x + crop_x, 2 * (g_id_y + crop_y) + 1), Y);
        write_imageui(img_uv, (int2)(g_id_x + crop_x, g_id_y + crop_y), UV);
    }
}
