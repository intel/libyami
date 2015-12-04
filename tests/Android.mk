LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
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

LOCAL_CFLAGS := \
         -O2 -fpermissive

ifeq ($(ENABLE-V4L2-OPS),true)
LOCAL_CFLAGS += -D__ENABLE_V4L2_OPS__
endif

LOCAL_MODULE := v4l2decoder
include $(BUILD_EXECUTABLE)
