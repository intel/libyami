LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
include $(LOCAL_PATH)/../common.mk

LOCAL_SRC_FILES := \
        vaapicodedbuffer.cpp \
        vaapiencpicture.cpp \
        vaapiencoder_base.cpp \
        vaapiencoder_host.cpp \

LOCAL_SRC_FILES += \
        vaapiencoder_h264.cpp \
        vaapilayerid.cpp

LOCAL_SRC_FILES += \
        vaapiencoder_jpeg.cpp

LOCAL_SRC_FILES += \
        vaapiencoder_vp8.cpp

LOCAL_SRC_FILES += \
        vaapiencoder_vp9.cpp

LOCAL_SRC_FILES += \
        vaapiencoder_hevc.cpp

LOCAL_C_INCLUDES:= \
        $(LOCAL_PATH)/.. \
        $(LOCAL_PATH)/../common \
        $(LOCAL_PATH)/../interface \
        $(LOCAL_PATH)/../vaapi \
        $(LOCAL_PATH)/../codecparsers \
        external/libcxx/include \
        $(TARGET_OUT_HEADERS)/libva \

LOCAL_CFLAGS := \
        -D__BUILD_H264_ENCODER__=1 \
        -D__BUILD_H265_ENCODER__=1 \
        -D__BUILD_VP8_ENCODER__=1 \
        -D__BUILD_VP9_ENCODER__=1 \
        -D__BUILD_JPEG_ENCODER__=1 \

LOCAL_SHARED_LIBRARIES := \
        liblog \
        libc++ \
        libva \

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := libyami_encoder
include $(BUILD_STATIC_LIBRARY)
