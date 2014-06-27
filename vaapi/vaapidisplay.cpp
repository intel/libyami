/*
 *  vaapidisplay.cpp - abstract for VADisplay
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "vaapi/vaapidisplay.h"

#include "common/log.h"
#include "vaapi/vaapiutils.h"
#include <list>

using std::tr1::shared_ptr;
using std::tr1::weak_ptr;
using std::list;

//helper function
static bool vaInit(VADisplay vaDisplay)
{
   int majorVersion, minorVersion;
   VAStatus vaStatus;
   vaStatus= vaInitialize(vaDisplay, &majorVersion, &minorVersion);
   return checkVaapiStatus(vaStatus, "vaInitialize");
}

typedef std::tr1::shared_ptr<Display> XDisplayPtr;
class VaapiX11Display:public VaapiDisplay
{
    friend DisplayPtr X11DisplayCreate(Display* display);

public:
    bool setRotation(int degree);


    ~VaapiX11Display();
protected:
    virtual bool isCompatible(const Display* other)
    {
        //NULL means any useful thing is ok
        if (!other)
            return true;
        return other == m_xDisplay.get();
    }

private:
    VaapiX11Display(const XDisplayPtr &, VADisplay);
    XDisplayPtr m_xDisplay;
};

struct XDisplayDeleter
{
    void operator()(Display* display) { XCloseDisplay(display); }
};

struct XDisplayNullDeleter
{
    void operator()(Display* display) { /*nothing*/ }
};

DisplayPtr X11DisplayCreate(Display* display)
{
    DisplayPtr ret;
    XDisplayPtr xDisplay;

    if (!display) {
        display = XOpenDisplay(NULL);
        if (!display) {
            ERROR("create local display failed");
            return ret;
        }
        xDisplay.reset(display, XDisplayDeleter());
    } else {
        xDisplay.reset(display, XDisplayNullDeleter());
    }

    VADisplay vaDisplay = vaGetDisplay(display);
    if (vaDisplay == NULL) {
        ERROR("vaGetDisplay failed.");
        return ret;
    }

    if (vaInit(vaDisplay))
        ret.reset(new VaapiX11Display(xDisplay, vaDisplay));
    return ret;
}

VaapiX11Display::VaapiX11Display(const XDisplayPtr& xDisplay, VADisplay vaDisplay)
:VaapiDisplay(vaDisplay), m_xDisplay(xDisplay)
{

}

VaapiX11Display::~VaapiX11Display()
{
    vaTerminate(m_display);
}

bool VaapiX11Display::setRotation(int degree)
{
    VAStatus vaStatus;
    VADisplayAttribute rotate;
    rotate.type = VADisplayAttribRotation;
    rotate.value = VA_ROTATION_NONE;
    if (degree == 0)
        rotate.value = VA_ROTATION_NONE;
    else if (degree == 90)
        rotate.value = VA_ROTATION_90;
    else if (degree == 180)
        rotate.value = VA_ROTATION_180;
    else if (degree == 270)
        rotate.value = VA_ROTATION_270;

    vaStatus = vaSetDisplayAttributes(m_display, &rotate, 1);
    if (!checkVaapiStatus(vaStatus, "vaSetDisplayAttributes"))
        return false;
    return true;

}



//display cache
class DisplayCache
{
public:
    static shared_ptr<DisplayCache> getInstance();
    template <typename UserData>
    DisplayPtr createDisplay(DisplayPtr (*create)(UserData), const UserData& data);

    ~DisplayCache() { pthread_mutex_destroy(&m_lock); }
private:
    DisplayCache() { pthread_mutex_init(&m_lock, NULL);}

    list<weak_ptr<VaapiDisplay> > m_cache;
    pthread_mutex_t m_lock;
};

shared_ptr<DisplayCache> DisplayCache::getInstance()
{
    static shared_ptr<DisplayCache> cache;
    if (!cache)
        cache.reset(new DisplayCache);
    return cache;
}

bool expired(const weak_ptr<VaapiDisplay>& weak)
{
    return !weak.lock();
}

template <typename UserData>
DisplayPtr DisplayCache::createDisplay(DisplayPtr (*create)(UserData), const UserData& data)
{
    pthread_mutex_lock(&m_lock);
    m_cache.remove_if(expired);

    //lockup first
    DisplayPtr display;
    list<weak_ptr<VaapiDisplay> >::iterator it;
    for (it = m_cache.begin(); it != m_cache.end(); ++it) {
        display = (*it).lock();
        if (display->isCompatible(data)) {
            pthread_mutex_unlock(&m_lock);
            return display;
        }
    }

    //crate new one
    display = create(data);
    if (display) {
        weak_ptr<VaapiDisplay> weak(display);
        m_cache.push_back(weak);
    }
    pthread_mutex_unlock(&m_lock);
    return display;
}


bool VaapiDisplay::setRotation(int degree)
{
    return true;
}

DisplayPtr VaapiDisplay::create(Display* display)
{
    return DisplayCache::getInstance()->createDisplay(X11DisplayCreate, display);
}

