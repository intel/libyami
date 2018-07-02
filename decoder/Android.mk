LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
include $(LOCAL_PATH)/../common.mk

LOCAL_SRC_FILES := \
        vaapidecoder_base.cpp \
        vaapidecoder_host.cpp \
        vaapidecsurfacepool.cpp \
        vaapidecpicture.cpp \

LOCAL_SRC_FILES += \
        vaapidecoder_h264.cpp

LOCAL_SRC_FILES += \
        vaapidecoder_h265.cpp

LOCAL_SRC_FILES += \
        vaapidecoder_vp8.cpp

LOCAL_SRC_FILES += \
        vaapidecoder_vp9.cpp

LOCAL_SRC_FILES += \
        vaapidecoder_vc1.cpp

LOCAL_SRC_FILES += \
        vaapidecoder_mpeg2.cpp

LOCAL_SRC_FILES += \
        vaapiDecoderJPEG.cpp

LOCAL_SRC_FILES += \
        vaapidecoder_fake.cpp

LOCAL_C_INCLUDES:= \
        $(LOCAL_PATH)/.. \
        $(LOCAL_PATH)/../common \
        $(LOCAL_PATH)/../interface \
        external/libcxx/include \
        $(TARGET_OUT_HEADERS)/libva \

LOCAL_CFLAGS := \
        -D__BUILD_H264_DECODER__=1 \
        -D__BUILD_H265_DECODER__=1 \
        -D__BUILD_VP8_DECODER__=1 \
        -D__BUILD_VP9_DECODER__=1 \
        -D__BUILD_VC1_DECODER__=1 \
        -D__BUILD_MPEG2_DECODER__=1 \
        -D__BUILD_JPEG_DECODER__=1 \


LOCAL_SHARED_LIBRARIES := \
        liblog \
        libc++ \
        libva \

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := libyami_decoder
include $(BUILD_STATIC_LIBRARY)
