LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	$(TEST_PATH)/../tests/decodeinput.cpp \
	$(TEST_PATH)/../tests/vppinputoutput.cpp \
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
        libyami \

LOCAL_CFLAGS := \
         -O2

LOCAL_MODULE := androidplayer
include $(BUILD_EXECUTABLE)
