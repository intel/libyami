LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
include $(LOCAL_PATH)/../common.mk

LOCAL_SRC_FILES := \
        vaapipicture.cpp \
        vaapibuffer.cpp \
        vaapiimage.cpp \
        vaapisurface.cpp\
        vaapiimageutils.cpp \
        vaapiutils.cpp \
        vaapidisplay.cpp \
        vaapicontext.cpp \
        vaapiimagepool.cpp \
        vaapisurfaceallocator.cpp \

LOCAL_C_INCLUDES:= \
        $(LOCAL_PATH)/.. \
        external/libcxx/include \
        $(TARGET_OUT_HEADERS)/libva

LOCAL_SHARED_LIBRARIES := \
        liblog \
        libva \
        libva-android \
        libc++ \

LOCAL_CPPFLAGS += \
        --rtti

LOCAL_MODULE := libyami_vaapi
include $(BUILD_STATIC_LIBRARY)
