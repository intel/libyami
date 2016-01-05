LOCAL_PATH := $(call my-dir)
LIBYAMICODEC_PATH := $(LOCAL_PATH)
include $(CLEAR_VARS)
include $(LIBYAMICODEC_PATH)/common.mk

include $(LIBYAMICODEC_PATH)/common/Android.mk
include $(LIBYAMICODEC_PATH)/codecparsers/Android.mk
include $(LIBYAMICODEC_PATH)/vaapi/Android.mk
include $(LIBYAMICODEC_PATH)/decoder/Android.mk
include $(LIBYAMICODEC_PATH)/encoder/Android.mk
include $(LIBYAMICODEC_PATH)/vpp/Android.mk
include $(LIBYAMICODEC_PATH)/v4l2/Android.mk

include $(CLEAR_VARS)
include $(LIBYAMICODEC_PATH)/common.mk

LOCAL_SHARED_LIBRARIES := \
        liblog \
        libva \
        libva-android \
        libc++ \

LOCAL_WHOLE_STATIC_LIBRARIES := \
        libyami_common \
        libcodecparser \
        libyami_vaapi \
        libyami_vpp \
        libyami_decoder \
        libyami_encoder \

LOCAL_MODULE := libyami

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
include $(LIBYAMICODEC_PATH)/common.mk
include $(LIBYAMICODEC_PATH)/examples/Android.mk
include $(LIBYAMICODEC_PATH)/tests/Android.mk
