LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
include $(LOCAL_PATH)/../common.mk

LOCAL_SRC_FILES := \
        vaapicodedbuffer.cpp \
        vaapiencpicture.cpp \
        vaapiencoder_base.cpp \
        vaapiencoder_host.cpp \

LOCAL_SRC_FILES += \
        vaapiencoder_h264.cpp

LOCAL_SRC_FILES += \
        vaapiencoder_jpeg.cpp

LOCAL_SRC_FILES += \
        vaapiencoder_vp8.cpp

#LOCAL_SRC_FILES += \
        vaapiencoder_hevc.cpp

LOCAL_C_INCLUDES:= \
        $(LOCAL_PATH)/.. \
        $(LOCAL_PATH)/../common \
        $(LOCAL_PATH)/../vaapi \
        $(LOCAL_PATH)/../codecparsers \
        external/libcxx/include \
        $(TARGET_OUT_HEADERS)/libva \

LOCAL_SHARED_LIBRARIES := \
        liblog \
        libc++

LOCAL_MODULE := libyami_encoder
include $(BUILD_STATIC_LIBRARY)
