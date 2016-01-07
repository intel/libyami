LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
include $(LOCAL_PATH)/../common.mk

LOCAL_SRC_FILES := \
        vaapipostprocess_base.cpp \
        vaapipostprocess_host.cpp \
        vaapipostprocess_scaler.cpp \
        vaapivpppicture.cpp

LOCAL_C_INCLUDES:= \
        $(LOCAL_PATH)/.. \
        $(LOCAL_PATH)/../interface \
        external/libcxx/include \
        $(TARGET_OUT_HEADERS)/libva

LOCAL_SHARED_LIBRARIES := \
        liblog \
        libva \
        libc++

LOCAL_CPPFLAGS += \
        --rtti

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

LOCAL_MODULE := libyami_vpp
include $(BUILD_STATIC_LIBRARY)
