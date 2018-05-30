LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
include $(LOCAL_PATH)/../common.mk

ROOT :=  $(LOCAL_PATH)/../
GEN :=  $(ROOT)/interface/YamiVersion.h
$(GEN) :  $(ROOT)/interface/YamiVersion.h.in $(ROOT)/configure.ac
	@python3  $(ROOT)/build/GenVer.py

LOCAL_GENERATED_SOURCES += $(GEN)

LOCAL_SRC_FILES := \
        log.cpp \
        utils.cpp \
        nalreader.cpp \
        surfacepool.cpp \
        PooledFrameAllocator.cpp \
        YamiVersion.cpp \

LOCAL_C_INCLUDES:= \
        $(LOCAL_PATH)/.. \
        $(LOCAL_PATH)/../interface \
        external/libcxx/include \
        $(TARGET_OUT_HEADERS)/libva \

LOCAL_SHARED_LIBRARIES := \
        liblog \
        libva

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := libyami_common
include $(BUILD_STATIC_LIBRARY)
