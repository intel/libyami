/*
 *  vaapiutils.h utils for vaapi
 *
 *  Copyright (C) 2016 Intel Corporation
 *    Author: Xu Guangxin <Guangxin.Xu@intel.com>
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
#ifndef vaapiutils_h
#define vaapiutils_h

#include "common/log.h"
#include <va/va.h>

#define checkVaapiStatus(status, prompt)                     \
    (                                                        \
        {                                                    \
            bool ret;                                        \
            ret = (status == VA_STATUS_SUCCESS);             \
            if (!ret)                                        \
                ERROR("%s: %s", prompt, vaErrorStr(status)); \
            ret;                                             \
        })

#endif //vaapiutils_h
