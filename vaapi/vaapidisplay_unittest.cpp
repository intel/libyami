/*
 * Copyright 2016 Intel Corporation
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

// The unittest header must be included before vaapidisplay.h.
// In vaapidisplay.h we include va_x11.h which includes Xlib.h and X.h.
// The X headers define 'Bool' and 'None' preprocessor types.  Gtest
// uses the same names to define some struct placeholders.  Thus, this
// creates a compile conflict if X defines them before gtest.  Hence, the
// include order requirement here is the only fix for this right now.
// See bug filed on gtest at https://github.com/google/googletest/issues/371
// for more details.
#include "common/unittest.h"

// primary header
#include "vaapidisplay.h"

// system headers
#include <fcntl.h>
#include <set>
#include <unistd.h>

namespace YamiMediaCodec {

class VaapiDisplayTest : public ::testing::Test {
    typedef std::set<intptr_t> DrmHandles;

protected:
    bool isCompatible(const DisplayPtr display, const NativeDisplay& native) {
        return display->isCompatible(native);
    }

    virtual void SetUp() {
#if defined(__ENABLE_X11__)
        m_x11_handle = 0;
#endif
    }

    intptr_t openDrmHandle() {
        intptr_t handle = -1;
        handle = open("/dev/dri/renderD128", O_RDWR);
        if (handle < 0)
            handle = open("/dev/dri/card0", O_RDWR);
        if (handle != -1)
            m_drm_handles.insert(handle);
        return handle;
    }

    void closeDrmHandle(intptr_t handle) {
        if (handle == -1)
            return;
        close(handle);
        const DrmHandles::const_iterator match(m_drm_handles.find(handle));
        if (match != m_drm_handles.end())
            m_drm_handles.erase(match);
    }

#if defined(__ENABLE_X11__)
    intptr_t openX11Handle() {
        m_x11_handle = (intptr_t)XOpenDisplay(NULL);
        return m_x11_handle;
    }
#endif

    virtual void TearDown() {
        const DrmHandles::const_iterator endIt(m_drm_handles.end());
        DrmHandles::const_iterator it;
        for (it = m_drm_handles.begin(); it != endIt; ++it)
            close(*it);
        m_drm_handles.clear();

#if defined(__ENABLE_X11__)
        if (m_x11_handle)
            XCloseDisplay((Display*)(m_x11_handle));
#endif
    }

    DrmHandles m_drm_handles;

#if defined(__ENABLE_X11__)
    intptr_t m_x11_handle;
#endif
};

#define VAAPI_DISPLAY_TEST(name) \
    TEST_F(VaapiDisplayTest, name)

#if defined(__ENABLE_X11__)
VAAPI_DISPLAY_TEST(CreateInvalidHandleX11) {
    //this turn off gest warning about multithread
    //you can check details at
    //https://github.com/google/googletest/blob/master/googletest/docs/AdvancedGuide.md
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";

    NativeDisplay native = {1, NATIVE_DISPLAY_X11};
    //this will crash vaGetDisplay
    ASSERT_DEATH({ DisplayPtr display = VaapiDisplay::create(native); }, "");
}
#endif

VAAPI_DISPLAY_TEST(CreateInvalidHandleDRM) {
    NativeDisplay native = {1, NATIVE_DISPLAY_DRM};
    DisplayPtr display = VaapiDisplay::create(native);

    EXPECT_FALSE(display.get());

    intptr_t handle = openDrmHandle();

    ASSERT_NE(handle, -1);

    // DRM handle is invalidated once it's closed.
    closeDrmHandle(handle);

    native.handle = handle;
    native.type = NATIVE_DISPLAY_DRM;
    display = VaapiDisplay::create(native);

    EXPECT_FALSE(display.get());
}

VAAPI_DISPLAY_TEST(CreateInvalidHandleVA) {
    NativeDisplay native = {0, NATIVE_DISPLAY_VA};
    DisplayPtr display = VaapiDisplay::create(native);

    EXPECT_FALSE(display.get());

    //this turn off gest warning about multithread
    //you can check details at
    //https://github.com/google/googletest/blob/master/googletest/docs/AdvancedGuide.md
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";

    native.handle = -1;
    native.type = NATIVE_DISPLAY_VA;

    //This will crash in vaDisplayIsValid
    ASSERT_DEATH({ display = VaapiDisplay::create(native); }, "");
}

#if defined(__ENABLE_X11__)
VAAPI_DISPLAY_TEST(CreateX11) {
    // Let yami create the native display
    NativeDisplay native = {0, NATIVE_DISPLAY_X11};
    DisplayPtr display = VaapiDisplay::create(native);

    ASSERT_TRUE(display.get());
    EXPECT_TRUE(display->getID() > 0);

    // Test the display caching
    DisplayPtr display2 = VaapiDisplay::create(native);

    ASSERT_TRUE(display2.get());
    EXPECT_TRUE(display.get() == display2.get());
    EXPECT_EQ(display->getID(), display2->getID());

    // DRM native type returns cached X11 display
    // TODO: Is this really supposed to be allowed?
    native.handle = 0;
    native.type = NATIVE_DISPLAY_DRM;
    display2 = VaapiDisplay::create(native);
    ASSERT_TRUE(display2.get());
    EXPECT_TRUE(display.get() == display2.get());
    EXPECT_EQ(display->getID(), display2->getID());

    // FIXME: If client created the DRM handle, it seems
    // that we should not get the cached X11 display, but we do.
    intptr_t handle = openDrmHandle();
    native.handle = handle;
    native.type = NATIVE_DISPLAY_DRM;
    display2 = VaapiDisplay::create(native);
    ASSERT_NE(handle, -1);
    ASSERT_TRUE(display2.get());
    EXPECT_FALSE(display.get() == display2.get());
    EXPECT_NE(display->getID(), display2->getID());

    // Let client create the native display
    native.handle = openX11Handle();
    native.type = NATIVE_DISPLAY_X11;
    display = VaapiDisplay::create(native);

    ASSERT_NE(m_x11_handle, -1);
    ASSERT_TRUE(display.get());
    EXPECT_TRUE(display->getID() > 0);

    // Ensure we didn't get the yami cached display
    EXPECT_FALSE(display.get() == display2.get());
    EXPECT_NE(display->getID(), display2->getID());
}
#endif

VAAPI_DISPLAY_TEST(CreateDRM) {
    // Let yami create the native display
    NativeDisplay native = {0, NATIVE_DISPLAY_DRM};
    DisplayPtr display = VaapiDisplay::create(native);

    ASSERT_TRUE(display.get());
    EXPECT_TRUE(display->getID() > 0);

    // Test the display caching
    DisplayPtr display2 = VaapiDisplay::create(native);

    ASSERT_TRUE(display2.get());
    EXPECT_TRUE(display.get() == display2.get());
    EXPECT_EQ(display->getID(), display2->getID());

    // Let client create the native display
    intptr_t handle = openDrmHandle();
    native.handle = handle;
    native.type = NATIVE_DISPLAY_DRM;
    display = VaapiDisplay::create(native);

    ASSERT_NE(handle, -1);
    ASSERT_TRUE(display.get());
    EXPECT_TRUE(display->getID() > 0);

    // Ensure we didn't get the yami cached display
    EXPECT_FALSE(display.get() == display2.get());
    EXPECT_NE(display->getID(), display2->getID());
}

VAAPI_DISPLAY_TEST(CreateVA) {
    NativeDisplay native = {1, NATIVE_DISPLAY_VA};
    DisplayPtr display = VaapiDisplay::create(native);

    ASSERT_TRUE(display.get());
    EXPECT_TRUE(display->getID() > 0);

    // Test the display caching
    DisplayPtr display2 = VaapiDisplay::create(native);

    ASSERT_TRUE(display2.get());
    EXPECT_TRUE(display.get() == display2.get());
    EXPECT_EQ(display->getID(), display2->getID());
}

#if defined(__ENABLE_X11__)
VAAPI_DISPLAY_TEST(CompatibleX11) {
    NativeDisplay native = {openX11Handle(), NATIVE_DISPLAY_X11};
    DisplayPtr display = VaapiDisplay::create(native);

    ASSERT_TRUE((Display*)m_x11_handle != NULL);
    ASSERT_TRUE(display.get());

    EXPECT_TRUE(isCompatible(display, native));

    native.handle = m_x11_handle + 1;
    native.type = NATIVE_DISPLAY_X11;
    EXPECT_FALSE(isCompatible(display, native));

    native.handle = 0;
    native.type = NATIVE_DISPLAY_X11;
    EXPECT_TRUE(isCompatible(display, native));

    native.handle = 0;
    native.type = NATIVE_DISPLAY_VA;
    EXPECT_FALSE(isCompatible(display, native));

    native.handle = 0;
    native.type = NATIVE_DISPLAY_DRM;
    EXPECT_TRUE(isCompatible(display, native));

    // FIXME: If client created the DRM handle, it seems
    // that we should not get the cached X11 display
    intptr_t handle = openDrmHandle();
    native.handle = handle;
    native.type = NATIVE_DISPLAY_DRM;
    ASSERT_NE(handle, -1);
    ASSERT_FALSE(isCompatible(display, native));

    native.handle = 0;
    native.type = NATIVE_DISPLAY_AUTO;
    EXPECT_TRUE(isCompatible(display, native));
}
#endif

VAAPI_DISPLAY_TEST(CompatibleDRM) {
    intptr_t handle = openDrmHandle();
    intptr_t handle2 = openDrmHandle();
    NativeDisplay native = {handle, NATIVE_DISPLAY_DRM};
    DisplayPtr display = VaapiDisplay::create(native);

    ASSERT_NE(handle, -1);
    ASSERT_NE(handle2, -1);
    ASSERT_NE(handle, handle2);
    ASSERT_TRUE(display.get());

    EXPECT_TRUE(isCompatible(display, native));

    native.handle = handle2;
    native.type = NATIVE_DISPLAY_DRM;
    EXPECT_FALSE(isCompatible(display, native));

    native.handle = 0;
    native.type = NATIVE_DISPLAY_DRM;
    EXPECT_TRUE(isCompatible(display, native));

    native.handle = -1;
    native.type = NATIVE_DISPLAY_DRM;
    EXPECT_TRUE(isCompatible(display, native));

    native.type = NATIVE_DISPLAY_AUTO;
    EXPECT_TRUE(isCompatible(display, native));

    native.handle = 0;
    native.type = NATIVE_DISPLAY_VA;
    EXPECT_FALSE(isCompatible(display, native));

    native.handle = 0;
    native.type = NATIVE_DISPLAY_X11;
    EXPECT_FALSE(isCompatible(display, native));
}

VAAPI_DISPLAY_TEST(CompatibleVA) {
    NativeDisplay native = {1, NATIVE_DISPLAY_VA};
    DisplayPtr display = VaapiDisplay::create(native);

    ASSERT_TRUE(display.get());

    EXPECT_TRUE(isCompatible(display, native));

    native.handle = 2;
    native.type = NATIVE_DISPLAY_VA;
    EXPECT_FALSE(isCompatible(display, native));

    native.handle = 0;
    native.type = NATIVE_DISPLAY_VA;
    EXPECT_FALSE(isCompatible(display, native));

    native.handle = -1;
    native.type = NATIVE_DISPLAY_VA;
    EXPECT_FALSE(isCompatible(display, native));

    native.handle = 1;
    native.type = NATIVE_DISPLAY_DRM;
    EXPECT_FALSE(isCompatible(display, native));

    native.handle = 1;
    native.type = NATIVE_DISPLAY_X11;
    EXPECT_FALSE(isCompatible(display, native));

    native.type = NATIVE_DISPLAY_AUTO;
    EXPECT_TRUE(isCompatible(display, native));
}

} //namespace YamiMediaCodec
