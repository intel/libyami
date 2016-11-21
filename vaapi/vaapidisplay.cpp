/*
 * Copyright (C) 2014 Intel Corporation. All rights reserved.
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
#include "vaapi/VaapiUtils.h"

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
    virtual ~NativeDisplayBase() {};
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

bool isInvalidDrmHandle(int handle)
{
    return handle == 0 || handle == -1;
}

#if defined(__ENABLE_X11__)
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
        if (display.type == NATIVE_DISPLAY_AUTO)
            return true;
        if (display.type == NATIVE_DISPLAY_DRM) {
            //invalid drm handle means any drm display is acceptable.
            if (isInvalidDrmHandle(display.handle))
                return true;
        }
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

        m_handle = open("/dev/dri/renderD128", O_RDWR);
        if (m_handle < 0)
            m_handle = open("/dev/dri/card0", O_RDWR);
        m_selfCreated = true;
        return m_handle != -1;
    };

    bool isCompatible(const NativeDisplay& display) {
        if (display.type == NATIVE_DISPLAY_AUTO)
            return true;
        if (display.type != NATIVE_DISPLAY_DRM)
            return false;
        if (isInvalidDrmHandle(display.handle) || display.handle == m_handle)
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

        if (acceptValidExternalHandle(display))
            return true;
        return vaDisplayIsValid((VADisplay)display.handle);
    };

    bool isCompatible(const NativeDisplay& display) {
        if (display.type == NATIVE_DISPLAY_AUTO)
            return true;
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
    if (!DynamicPointerCast<NativeDisplayVADisplay>(m_nativeDisplay)) {
        vaTerminate(m_vaDisplay);
    }
}

bool VaapiDisplay::setRotation(int degree)
{
    VAStatus vaStatus;
    VADisplayAttribute* attrList = NULL;
    VADisplayAttribute rotate;
    int i, numAttributes;
    rotate.type = VADisplayAttribRotation;
    rotate.value = VA_ROTATION_NONE;
    if (degree == 0) //no need to set rotation when degree is zero
        return true;
    else if (degree == 90)
        rotate.value = VA_ROTATION_90;
    else if (degree == 180)
        rotate.value = VA_ROTATION_180;
    else if (degree == 270)
        rotate.value = VA_ROTATION_270;

    /* should query before set display attributres */
    vaStatus = vaQueryDisplayAttributes(m_vaDisplay, attrList, &numAttributes);
    if (!checkVaapiStatus(vaStatus, "vaQueryDisplayAttributes") || !attrList)
       return false;

    for (i = 0; i < numAttributes; i++) {
        if (attrList[i].type == VADisplayAttribRotation)
            break;
    }

    if ((i == numAttributes) || !(attrList[i].flags & VA_DISPLAY_ATTRIB_SETTABLE) )
        return false;

    vaStatus = vaSetDisplayAttributes(m_vaDisplay, &rotate, 1);
    if (!checkVaapiStatus(vaStatus, "vaSetDisplayAttributes"))
        return false;
    return true;

}

const VAImageFormat *
VaapiDisplay::getVaFormat(uint32_t fourcc)
{
    AutoLock locker(m_lock);
    size_t i;

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
    DisplayPtr createDisplay(const NativeDisplay& nativeDisplay);

    ~DisplayCache() {}
private:
    DisplayCache() {}

    list<WeakPtr<VaapiDisplay> > m_cache;
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

bool expired(const WeakPtr<VaapiDisplay>& weak)
{
    return !weak.lock();
}

DisplayPtr DisplayCache::createDisplay(const NativeDisplay& nativeDisplay)
{
    NativeDisplayPtr nativeDisplayObj;
    VADisplay vaDisplay = NULL;
    DisplayPtr vaapiDisplay;
    YamiMediaCodec::AutoLock locker(m_lock);

    m_cache.remove_if(expired);

    //lockup first
    list<WeakPtr<VaapiDisplay> >::iterator it;
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
#if defined(__ENABLE_X11__)
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
#ifndef ANDROID
            vaDisplay = vaGetDisplayDRM(nativeDisplayObj->nativeHandle());
#endif
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

    if (nativeDisplay.type == NATIVE_DISPLAY_VA || vaInit(vaDisplay)) {
        DisplayPtr temp(new VaapiDisplay(nativeDisplayObj, vaDisplay));
        vaapiDisplay = temp;
    }
    if (vaapiDisplay) {
        WeakPtr<VaapiDisplay> weak(vaapiDisplay);
        m_cache.push_back(weak);
    }
    return vaapiDisplay;
}

DisplayPtr VaapiDisplay::create(const NativeDisplay& display)
{
    return DisplayCache::getInstance()->createDisplay(display);
}
} //YamiMediaCodec
