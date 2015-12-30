LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
include $(LOCAL_PATH)/../common.mk

LOCAL_SRC_FILES := \
        vaapidecoder_base.cpp \
        vaapidecoder_host.cpp \
        vaapidecsurfacepool.cpp \
        vaapidecpicture.cpp \

LOCAL_SRC_FILES += \
        vaapidecoder_h264.cpp \
        vaapidecoder_h264_dpb.cpp \

#LOCAL_SRC_FILES += \
        vaapidecoder_h265.cpp

LOCAL_SRC_FILES += \
        vaapidecoder_vp8.cpp

LOCAL_SRC_FILES += \
        vaapidecoder_vp9.cpp

LOCAL_SRC_FILES += \
        vaapidecoder_jpeg.cpp

LOCAL_SRC_FILES += \
        vaapidecoder_fake.cpp

LOCAL_C_INCLUDES:= \
        $(LOCAL_PATH)/.. \
        $(LOCAL_PATH)/../common \
        external/libcxx/include \
        $(TARGET_OUT_HEADERS)/libva \

LOCAL_SHARED_LIBRARIES := \
        liblog \
        libc++

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

LOCAL_MODULE := libyami_decoder
include $(BUILD_STATIC_LIBRARY)
