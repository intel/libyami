/*
 *  vaapidisplay.h - abstract for VADisplay
 *
 *  Copyright (C) 2014 Intel Corporation
 *    Author: Xu Guangxin <guangxin.xu@intel.com>
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

#ifndef vaapidisplay_h
#define vaapidisplay_h

#include "vaapi/vaapiptrs.h"
#include "vaapi/vaapitypes.h"
#include <va/va.h>
#include <va/va_tpi.h>
#ifdef HAVE_VA_X11
#include <va/va_x11.h>
#endif

///abstract for all display, x11, wayland, ozone, android etc.
class VaapiDisplay
{
friend class DisplayCache;
public:
    //FIXME: add more create functions.
    static DisplayPtr create(Display*);

    virtual bool setRotation(int degree);

    VADisplay getID() const { return m_display; }

protected:
    /// for display cache management.
    virtual bool isCompatible(const Display*) {return false;}

    VaapiDisplay(VADisplay vaDisplay):m_display(vaDisplay){}
    VADisplay   m_display;
    DISALLOW_COPY_AND_ASSIGN(VaapiDisplay);
};

#endif

