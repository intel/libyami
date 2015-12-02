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

#ifndef vaapidisplay_h
#define vaapidisplay_h

#include "vaapi/vaapiptrs.h"
#include <va/va.h>
#include <va/va_tpi.h>
#ifdef HAVE_VA_X11
#include <va/va_x11.h>
#endif
#ifndef ANDROID
#include <va/va_drm.h>
#endif
#include <vector>
#include "common/lock.h"
#include "common/NonCopyable.h"

///abstract for all display, x11, wayland, ozone, android etc.
namespace YamiMediaCodec{
#ifndef VA_FOURCC_I420
#define VA_FOURCC_I420 VA_FOURCC('I','4','2','0')
#endif
class NativeDisplayBase;
class VaapiDisplay
{
    typedef SharedPtr<NativeDisplayBase> NativeDisplayPtr;
    friend class DisplayCache;
    friend class VaapiDisplayTest;

public:
    virtual ~VaapiDisplay();
    //FIXME: add more create functions.
    static DisplayPtr create(const NativeDisplay& display);
    virtual bool setRotation(int degree);
    VADisplay getID() const { return m_vaDisplay; }
    const VAImageFormat* getVaFormat(uint32_t fourcc);

protected:
    /// for display cache management.
    virtual bool isCompatible(const NativeDisplay& other);

private:
    VaapiDisplay(const NativeDisplayPtr& nativeDisplay, VADisplay vaDisplay)
    :m_vaDisplay(vaDisplay), m_nativeDisplay(nativeDisplay) { };

    Lock m_lock;
    VADisplay   m_vaDisplay;
    NativeDisplayPtr m_nativeDisplay;
    std::vector<VAImageFormat> m_vaImageFormats;

DISALLOW_COPY_AND_ASSIGN(VaapiDisplay);
};
}
#endif

