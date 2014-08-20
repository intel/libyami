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
#include <va/va_drm.h>
#include "interface/VideoCommonDefs.h"

///abstract for all display, x11, wayland, ozone, android etc.
class NativeDisplayBase;
class VaapiDisplay
{
    typedef std::tr1::shared_ptr<NativeDisplayBase> NativeDisplayPtr;
    friend class DisplayCache;

public:
    ~VaapiDisplay();
    //FIXME: add more create functions.
    static DisplayPtr create(const NativeDisplay& display);

    virtual bool setRotation(int degree);

    VADisplay getID() const { return m_vaDisplay; }

protected:
    /// for display cache management.
    virtual bool isCompatible(const NativeDisplay& other);

private:
    VaapiDisplay(const NativeDisplayPtr& nativeDisplay, VADisplay vaDisplay)
    :m_vaDisplay(vaDisplay), m_nativeDisplay(nativeDisplay) { };

    VADisplay   m_vaDisplay;
    NativeDisplayPtr m_nativeDisplay;

DISALLOW_COPY_AND_ASSIGN(VaapiDisplay);
};

#endif

