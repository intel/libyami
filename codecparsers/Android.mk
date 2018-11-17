LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
include $(LOCAL_PATH)/../common.mk

LOCAL_SRC_FILES := \
        bitReader.cpp \
        bitWriter.cpp \
        EpbReader.cpp \
        nalReader.cpp \
        h264Parser.cpp \
        h265Parser.cpp \
        jpegParser.cpp \
        mpeg2_parser.cpp \
        vc1Parser.cpp \
        vp8_bool_decoder.cpp \
        vp8_parser.cpp \
        vp9parser.cpp \
        vp9quant.c \
        dboolhuff.c \

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/.. \
        $(LOCAL_PATH)/../interface

LOCAL_SHARED_LIBRARIES := \
        libcutils \
        liblog

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := libcodecparser
include $(BUILD_STATIC_LIBRARY)
