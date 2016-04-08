/*
 * Copyright (c) 2015-2016 Intel Corporation. All rights reserved.
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
#ifndef bumpbox_h
#define bumpbox_h
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
/// class to hide box bump logical
/// the inner rect will bump back when it hit outerrect.
///
class BumpBox
{
public:

    BumpBox(uint32_t outerWidth, uint32_t outerHeight, uint32_t width, uint32_t height, uint32_t step = 5)
        :m_width(width), m_height(height)
    {
        assert(outerWidth > width && outerHeight > height);
        m_xMax = outerWidth - width;
        m_yMax = outerHeight - height;
        m_x = rand() % m_xMax;
        m_y = rand() % m_yMax;

        //generate dx, dy
        uint32_t a = rand()%255 + 1;
        uint32_t b = rand()%255 + 1;
        double c2 = a*a + b*b;
        double c = sqrt(c2);
        m_dx = genSign() * (a / c * step);
        m_dy = genSign() * (b / c * step);
        //        printf("(%d, %d, %d, %d)\r\n", outerWidth, outerWidth, width, height);
        //printf("a = %d, b = %d, c = %f, a/c = %f\r\n", a, b, c, a/c);
    }
    void getPos(uint32_t& x, uint32_t& y, uint32_t& width, uint32_t& height)
    {
        width = m_width;
        height = m_height;
        //step
        m_x += m_dx;
        m_y += m_dy;

        //clip
        if (clip(m_x, 0, m_xMax))
            m_dx = -m_dx;
        if  (clip(m_y, 0, m_yMax))
            m_dy = -m_dy;
        //now we get the pos
        x = m_x;
        y = m_y;

    }
private:
    bool clip(int& a, int left, int right)
    {
        if (a <= left) {
            a = left;
            return true;
        }
        else if (a >= right) {
            a = right;
            return true;
        }
        return false;

    }
    int genSign()
    {
        int ret = rand() % 2;
        if (!ret)
            ret = -1;
        return ret;
    }
    int32_t m_x;
    int32_t m_y;
    double m_dx;
    double m_dy;
    uint32_t m_xMax;
    uint32_t m_yMax;
    uint32_t m_width;
    uint32_t m_height;

};

#endif
