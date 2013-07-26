LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
        vaapiutils.cpp        \
        vaapisurface.cpp      \
        vaapiimage.cpp        \
        vaapibuffer.cpp \
        vaapisurface_pool.cpp \

LOCAL_C_INCLUDES:= \
        $(TARGET_OUT_HEADERS)/libva \
        $(TOP)/frameworks/base/include

LOCAL_SHARED_LIBRARIES :=       \
        libutils                \
        liblog                  \
        libcutils               \
        libva                   \
        libva-android           \
        libva-tpi               \

LOCAL_MODULE := libvacodec_common
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

