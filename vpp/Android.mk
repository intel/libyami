LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
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
        libc++ \

LOCAL_CFLAGS := \
        -O2 --rtti

LOCAL_MODULE := libyami_vpp
include $(BUILD_STATIC_LIBRARY)
