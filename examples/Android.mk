LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
include $(LOCAL_PATH)/../common.mk

LOCAL_SRC_FILES := \
    ../tests/decodeinput.cpp \
    ../tests/vppinputoutput.cpp \
    androidplayer.cpp

LOCAL_C_INCLUDES:= \
        $(LOCAL_PATH)/.. \
        $(LOCAL_PATH)/../interface \
        $(LOCAL_PATH)/../tests \
        external/libcxx/include \
        $(TARGET_OUT_HEADERS)/libva

LOCAL_SHARED_LIBRARIES := \
        libutils \
        liblog \
        libc++ \
        libva \
        libva-android \
        libgui \
        libhardware \
        libyami \

LOCAL_MODULE := androidplayer
include $(BUILD_EXECUTABLE)
