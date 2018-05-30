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
include $(LIBYAMICODEC_PATH)/capi/Android.mk

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
        libyami_capi \

LOCAL_EXPORT_C_INCLUDE_DIRS := \
        $(LIBYAMICODEC_PATH)/interface

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := libyami

include $(BUILD_SHARED_LIBRARY)
