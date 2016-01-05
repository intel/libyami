LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
include $(LOCAL_PATH)/../common.mk

LOCAL_SRC_FILES := \
        bitreader.c \
        bytereader.c \
        bytewriter.c \
        h264parser.c \
        h265parser.c \
        mpegvideoparser.c \
        mpeg4parser.c \
        vc1parser.c \
        vp8utils.c \
        vp8rangedecoder.c \
        vp8parser.c \
        vp9quant.c \
        vp9parser.c\
        dboolhuff.c \
        jpegparser.c \
        parserutils.c \
        nalutils.c \
        bitwriter.c

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/.. \

LOCAL_SHARED_LIBRARIES := \
        libcutils \
        liblog

LOCAL_MODULE := libcodecparser
include $(BUILD_STATIC_LIBRARY)
