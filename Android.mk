LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LIBVACODEC_PATH := $(LOCAL_PATH)

include $(LIBVACODEC_PATH)/codecparsers/Android.mk
include $(LIBVACODEC_PATH)/common/Android.mk
include $(LIBVACODEC_PATH)/decoder/Android.mk

