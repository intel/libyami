LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CFLAGS := $(VERSION_CFLAGS)

#LOCAL_CFLAGS += -DLOG_NDEBUG=0

#LOCAL_C_INCLUDES:= \

LOCAL_SRC_FILES := \
    bitreader.c       \
    bytereader.c      \
    nalreader.c       \
    parserutils.c     \
    h264parser.c      \
    vc1parser.c       \
    mpeg4parser.c     \
    mpegvideoparser.c 

LOCAL_C_INCLUDES:= \
        $(TOP)/frameworks/base/include \

LOCAL_SHARED_LIBRARIES := \
    libcutils  \
    liblog
   

LOCAL_MODULE := libcodecparsers
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

