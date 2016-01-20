LOCAL_PATH := $(call my-dir)

## v4l2decoder
include $(CLEAR_VARS)
include $(LOCAL_PATH)/../common.mk

LOCAL_SRC_FILES := \
        decodehelp.cpp \
        decodeinput.cpp \
        vppinputoutput.cpp \
        v4l2decode.cpp

LOCAL_C_INCLUDES:= \
        $(LOCAL_PATH)/.. \
        external/libcxx/include \
        $(LOCAL_PATH)/../interface \
        $(TARGET_OUT_HEADERS)/libva

LOCAL_SHARED_LIBRARIES := \
        libyami_v4l2 \
        libutils \
        libgui \

ifeq ($(ENABLE-V4L2-OPS),true)
LOCAL_SHARED_LIBRARIES += libdl
endif

LOCAL_CPPFLAGS += \
         -fpermissive

ifeq ($(ENABLE-V4L2-OPS),true)
LOCAL_CPPFLAGS += -D__ENABLE_V4L2_OPS__
endif

LOCAL_MODULE := v4l2decode
include $(BUILD_EXECUTABLE)

## v4l2encoder
include $(CLEAR_VARS)
include $(LOCAL_PATH)/../common.mk

LOCAL_SRC_FILES := \
        encodeinput.cpp \
        encodeInputSurface.cpp \
        v4l2encode.cpp

LOCAL_C_INCLUDES:= \
        $(LOCAL_PATH)/.. \
        external/libcxx/include \
        $(LOCAL_PATH)/../interface \
        $(TARGET_OUT_HEADERS)/libva

LOCAL_SHARED_LIBRARIES := \
        libyami_v4l2 \
        libutils \
        libgui \
        libui \
        liblog

LOCAL_CPPFLAGS += \
         -fpermissive

LOCAL_MODULE := v4l2encode
include $(BUILD_EXECUTABLE)
