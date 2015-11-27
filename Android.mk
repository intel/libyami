LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LIBYAMICODEC_PATH := $(LOCAL_PATH)
include $(LIBYAMICODEC_PATH)/common/Android.mk
include $(LIBYAMICODEC_PATH)/codecparsers/Android.mk
include $(LIBYAMICODEC_PATH)/vaapi/Android.mk
include $(LIBYAMICODEC_PATH)/decoder/Android.mk
include $(LIBYAMICODEC_PATH)/encoder/Android.mk
include $(LIBYAMICODEC_PATH)/vpp/Android.mk
include $(LIBYAMICODEC_PATH)/examples/Android.mk
