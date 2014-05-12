/*
 *  vaapiptrs.h - shortcuts for smart pointers.
 *
 *  Copyright (C) 2013 Intel Corporation
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

#ifndef vaapiptrs_h
#define vaapiptrs_h
#include <tr1/memory>

class VaapiSurface;
typedef std::tr1::shared_ptr < VaapiSurface > SurfacePtr;

class VaapiCodedBuffer;
typedef std::tr1::shared_ptr < VaapiCodedBuffer > CodedBufferPtr;

class VaapiBufObject;
typedef std::tr1::shared_ptr < VaapiBufObject > BufObjectPtr;

#endif                          /* vaapiptr_h */
