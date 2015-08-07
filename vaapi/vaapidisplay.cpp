/*
 *  vaapidisplay.cpp - abstract for VADisplay
 *
 *  Copyright (C) 2014 Intel Corporation
 *    Author: Xu Guangxin <guangxin.xu@intel.com>
 *            Zhao Halley<halley.zhao@intel.com>
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
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <list>
#include "common/log.h"
#include "common/lock.h"
#include "vaapi/vaapiutils.h"


using std::tr1::weak_ptr;
using std::list;
/**
1. client and yami shares the same display when it provides both native
   display (x11 Display or drm fd) and display type in setNativeDisplay().
   client owns the native display
2. when client set native display type only, yami will create a native
   display and owns it.
3. when client doesn't set any native display, or set display type to
   NATIVE_DISPLAY_AUTO; yami will try to open a x11 display first. if fail,
   drm display will try next

4. A x11 display is compatible to drm display, while not reverse.
   it means, when there is a VADisplay created from x11 display already ,
   yami will not create a new display when drm display is requested,
   simplely reuse the VADisplay.
*/

//helper function
namespace YamiMediaCodec{
static bool vaSelectDriver(VADisplay vaDisplay, const std::string& driverName)
{
    VAStatus vaStatus=VA_STATUS_SUCCESS;
    if (!driverName.empty())
    {
        vaStatus = vaSetDriverName(vaDisplay, const_cast<char*>(driverName.c_str()));
    }
    return checkVaapiStatus(vaStatus, "vaSetDriverName");
}

static bool vaInit(VADisplay vaDisplay)
{
   int majorVersion, minorVersion;
   VAStatus vaStatus;
   vaStatus= vaInitialize(vaDisplay, &majorVersion, &minorVersion);
   return checkVaapiStatus(vaStatus, "vaInitialize");
}

class NativeDisplayBase {
  public:
    NativeDisplayBase() :m_handle(0) { };
    ~NativeDisplayBase() {};
    virtual bool initialize(const NativeDisplay& display) = 0;
    virtual bool isCompatible (const NativeDisplay& display) = 0;
    intptr_t nativeHandle() {return m_handle;};

  protected:
    virtual bool acceptValidExternalHandle(const NativeDisplay& display) {
        if (display.handle && display.handle != -1) {
            m_handle = display.handle;
            m_selfCreated = false;
            return true;
        }
        return false;
    }
    intptr_t m_handle;
    bool m_selfCreated;

  private:
    DISALLOW_COPY_AND_ASSIGN(NativeDisplayBase);
};

#if __ENABLE_X11__
class NativeDisplayX11 : public NativeDisplayBase{
  public:
    NativeDisplayX11() :NativeDisplayBase(){ };
    virtual ~NativeDisplayX11() {
        if (m_selfCreated && m_handle)
            XCloseDisplay((Display*)(m_handle));
    };

    virtual bool initialize (const NativeDisplay& display) {
        ASSERT(display.type == NATIVE_DISPLAY_X11 || display.type == NATIVE_DISPLAY_AUTO);

        if (acceptValidExternalHandle(display))
            return true;

        m_handle = (intptr_t)XOpenDisplay(NULL);
        m_selfCreated = true;
        return (Display*)m_handle != NULL;
    };

    virtual bool isCompatible(const NativeDisplay& display) {
        if (display.type == NATIVE_DISPLAY_DRM || display.type == NATIVE_DISPLAY_AUTO)
            return true; // x11 is compatile to any drm, (do not consider one X with 2 gfx device)
        if (display.type == NATIVE_DISPLAY_X11 && (!display.handle || display.handle == m_handle))
            return true;
        return false;
    }
};
#endif
class NativeDisplayDrm : public NativeDisplayBase{
  public:
    NativeDisplayDrm() :NativeDisplayBase(){ };
    virtual ~NativeDisplayDrm() {
        if (m_selfCreated && m_handle && m_handle != -1)
            ::close(m_handle);
    };
    virtual bool initialize (const NativeDisplay& display) {
        ASSERT(display.type == NATIVE_DISPLAY_DRM || display.type == NATIVE_DISPLAY_AUTO);

        if (acceptValidExternalHandle(display))
            return true;

        m_handle = open("/dev/dri/card0", O_RDWR);
        m_selfCreated = true;
        return m_handle != -1;
    };

    bool isCompatible(const NativeDisplay& display) {
        if (display.type != NATIVE_DISPLAY_DRM)
            return false;
        if (display.handle == 0 || display.handle == -1 || display.handle == m_handle)
            return true;
        return false;
    }
};

class NativeDisplayVADisplay : public NativeDisplayBase{
  public:
    NativeDisplayVADisplay() :NativeDisplayBase(){ };
    ~NativeDisplayVADisplay() {};
    virtual bool initialize (const NativeDisplay& display) {
        ASSERT(display.type == NATIVE_DISPLAY_VA);

        return acceptValidExternalHandle(display);
    };

    bool isCompatible(const NativeDisplay& display) {
        return display.type == NATIVE_DISPLAY_VA && display.handle == m_handle;
    }
};

typedef SharedPtr<NativeDisplayBase> NativeDisplayPtr;

bool VaapiDisplay::isCompatible(const NativeDisplay& other)
{
    return m_nativeDisplay->isCompatible(other);
}

VaapiDisplay::~VaapiDisplay()
{
    vaTerminate(m_vaDisplay);
}

bool VaapiDisplay::setRotation(int degree)
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

    vaStatus = vaSetDisplayAttributes(m_vaDisplay, &rotate, 1);
    if (!checkVaapiStatus(vaStatus, "vaSetDisplayAttributes"))
        return false;
    return true;

}

const VAImageFormat *
VaapiDisplay::getVaFormat(uint32_t fourcc)
{
    AutoLock locker(m_lock);
    int i;

    if (m_vaImageFormats.empty()) {
        VAStatus vaStatus;
        int numImageFormats;
        numImageFormats = vaMaxNumImageFormats(m_vaDisplay);
        if (numImageFormats == 0)
            return NULL;

        m_vaImageFormats.reserve(numImageFormats);
        m_vaImageFormats.resize(numImageFormats);

        vaStatus = vaQueryImageFormats(m_vaDisplay, &m_vaImageFormats[0], &numImageFormats);
        checkVaapiStatus(vaStatus, "vaQueryImageFormats()");
        for (i=0; i< m_vaImageFormats.size(); i++)
            DEBUG_FOURCC("supported image format: ", m_vaImageFormats[i].fourcc);
    }

    for (i = 0; i < m_vaImageFormats.size(); i++) {
        VAImageFormat vaImageFormat = m_vaImageFormats[i];
        if (vaImageFormat.fourcc == fourcc)
            return &m_vaImageFormats[i];
    }
    return NULL;
}

//display cache
class DisplayCache
{
public:
    static SharedPtr<DisplayCache> getInstance();
    DisplayPtr createDisplay(const NativeDisplay& nativeDisplay, const std::string& driverName="");

    ~DisplayCache() {}
private:
    DisplayCache() {}

    list<weak_ptr<VaapiDisplay> > m_cache;
    YamiMediaCodec::Lock m_lock;
};

SharedPtr<DisplayCache> DisplayCache::getInstance()
{
    static SharedPtr<DisplayCache> cache;
    if (!cache) {
        SharedPtr<DisplayCache> temp(new DisplayCache);
        cache = temp;
    }
    return cache;
}

bool expired(const weak_ptr<VaapiDisplay>& weak)
{
    return !weak.lock();
}

DisplayPtr DisplayCache::createDisplay(const NativeDisplay& nativeDisplay, const std::string& driverName)
{
    NativeDisplayPtr nativeDisplayObj;
    VADisplay vaDisplay = NULL;
    DisplayPtr vaapiDisplay;
    YamiMediaCodec::AutoLock locker(m_lock);

    m_cache.remove_if(expired);

    //lockup first
    list<weak_ptr<VaapiDisplay> >::iterator it;
    for (it = m_cache.begin(); it != m_cache.end(); ++it) {
        vaapiDisplay = (*it).lock();
        if (vaapiDisplay->isCompatible(nativeDisplay)) {
            return vaapiDisplay;
        }
    }
    vaapiDisplay.reset();

    //crate new one
    DEBUG("nativeDisplay: (type : %d), (handle : %ld)", nativeDisplay.type, nativeDisplay.handle);

    switch (nativeDisplay.type) {
    case NATIVE_DISPLAY_AUTO:
#if __ENABLE_X11__
    case NATIVE_DISPLAY_X11:
        nativeDisplayObj.reset (new NativeDisplayX11());
        if (nativeDisplayObj && nativeDisplayObj->initialize(nativeDisplay))
            vaDisplay = vaGetDisplay((Display*)(nativeDisplayObj->nativeHandle()));
        if (vaDisplay)
            INFO("use vaapi x11 backend");
        if(vaDisplay || nativeDisplay.type == NATIVE_DISPLAY_X11)
            break;
        INFO("try to init va with x11 display (%p) but failed", (Display*)(nativeDisplayObj->nativeHandle()));
        // NATIVE_DISPLAY_AUTO continue, no break
#endif
    case NATIVE_DISPLAY_DRM:
        nativeDisplayObj.reset (new NativeDisplayDrm());
        if (nativeDisplayObj && nativeDisplayObj->initialize(nativeDisplay))
            vaDisplay = vaGetDisplayDRM(nativeDisplayObj->nativeHandle());
        INFO("use vaapi drm backend");
        break;
    case NATIVE_DISPLAY_VA:
        nativeDisplayObj.reset (new NativeDisplayVADisplay());
        if (nativeDisplayObj && nativeDisplayObj->initialize(nativeDisplay))
            vaDisplay = (VADisplay)nativeDisplayObj->nativeHandle();
        INFO("use vaapi va backend");
        break;
    default:
        break;
    }

    if (vaDisplay == NULL) {
        ERROR("vaGetDisplay failed.");
        return vaapiDisplay;
    }

    if (!driverName.empty())
        vaSelectDriver(vaDisplay, driverName);

    if (nativeDisplay.type == NATIVE_DISPLAY_VA || vaInit(vaDisplay)) {
        DisplayPtr temp(new VaapiDisplay(nativeDisplayObj, vaDisplay));
        vaapiDisplay = temp;
    }
    if (vaapiDisplay) {
        weak_ptr<VaapiDisplay> weak(vaapiDisplay);
        m_cache.push_back(weak);
    }
    return vaapiDisplay;
}

DisplayPtr VaapiDisplay::create(const NativeDisplay& display)
{
    return DisplayCache::getInstance()->createDisplay(display);
}

DisplayPtr VaapiDisplay::create(const NativeDisplay& display, VAProfile profile)
{
    SharedPtr<DisplayCache> cache = DisplayCache::getInstance();
    const std::string name="hybrid";
    if (profile == VAProfileVP9Profile0)
        return cache->createDisplay(display, name);
    else
        return cache->createDisplay(display);
}
} //YamiMediaCodec
