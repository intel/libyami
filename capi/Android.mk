LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
include $(LOCAL_PATH)/../common.mk

LOCAL_SRC_FILES := \
        VideoDecoderCapi.cpp \
        VideoEncoderCapi.cpp \

LOCAL_C_INCLUDES:= \
        $(LOCAL_PATH)/.. \
        $(LOCAL_PATH)/../common \
        $(LOCAL_PATH)/../interface \
        external/libcxx/include \
        $(TARGET_OUT_HEADERS)/libva \

LOCAL_SHARED_LIBRARIES := \
        liblog \
        libc++

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := libyami_capi
include $(BUILD_STATIC_LIBRARY)
