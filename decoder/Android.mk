LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

APP_STL := stlport_shared

LOCAL_SRC_FILES := \
        vaapipicture.cpp         \
        vaapisurfacebuf_pool.cpp \
        vaapidecoder_host.cpp    \
        vaapidecoder_base.cpp    \
        vaapidecoder_h264_dpb.cpp \
        vaapidecoder_h264.cpp  

LOCAL_C_INCLUDES:= \
        $(TARGET_OUT_HEADERS)/libva \
        $(TOP)/frameworks/base/include \
        $(LOCAL_PATH)/.. \

LOCAL_SHARED_LIBRARIES :=       \
        liblog                  \
        libva                   \
        libva-android           \
        libva-tpi               \
        libcodecparsers         \
        libyami_common

LOCAL_C_INCLUDES += \
        external/stlport/stlport \
        bionic/ \
        bionic/libstdc++/include

LOCAL_SHARED_LIBRARIES += libstlport

LOCAL_MODULE := libyami_decoder
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

