INCLUDES = -I$(top_srcdir)

if ENABLE_CAPI
bin_PROGRAMS = decodecapi encodecapi
else
bin_PROGRAMS = yamidecode yamiencode yamivpp yamitranscode
if ENABLE_V4L2
bin_PROGRAMS += v4l2encode v4l2decode
endif
endif

AM_CFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/interface \
	$(LIBVA_CFLAGS) \
	$(NULL)

DECODE_INPUT_SOURCES = \
	decodeinput.cpp \
	$(NULL)

if ENABLE_AVFORMAT
DECODE_INPUT_SOURCES += decodeinputavformat.cpp
AM_CFLAGS += -D__STDC_CONSTANT_MACROS \
	$(LIBAVFORMAT_CFLAGS)
endif

AM_CPPFLAGS = $(AM_CFLAGS)

YAMI_COMMON_LIBS = \
	$(LIBVA_LIBS) \
	$(top_builddir)/common/libyami_common.la \
	$(top_builddir)/vaapi/libyami_vaapi.la \
	$(NULL)

if ENABLE_X11
YAMI_COMMON_LIBS += $(LIBVA_X11_LIBS) -lX11
endif

YAMI_DECODE_LIBS = \
	$(YAMI_COMMON_LIBS) \
	$(top_builddir)/decoder/libyami_decoder.la \
	$(NULL)

if BUILD_STATIC
YAMI_DECODE_LDFLAGS = \
	-Wl,--whole-archive \
	-Wl,$(top_builddir)/decoder/.libs/libyami_decoder.a \
	-Wl,--no-whole-archive
endif

if ENABLE_TESTS_GLES
YAMI_DECODE_LIBS += $(LIBEGL_LIBS) $(LIBGLES2_LIBS)
endif

if ENABLE_AVFORMAT
YAMI_DECODE_LIBS += $(LIBAVFORMAT_LIBS)
endif

if ENABLE_MD5
YAMI_DECODE_LIBS += -lcrypto
endif

YAMI_ENCODE_LIBS = \
	$(YAMI_COMMON_LIBS) \
	$(top_builddir)/encoder/libyami_encoder.la \
	$(YAMI_DECODE_LIBS) \
	$(NULL)

if BUILD_STATIC
YAMI_ENCODE_LDFLAGS = \
	-Wl,--whole-archive \
	-Wl,$(top_builddir)/encoder/.libs/libyami_encoder.a \
	-Wl,--no-whole-archive
endif

V4L2_DECODE_LIBS = \
	$(YAMI_DECODE_LIBS) \
	$(top_builddir)/v4l2/libyami_v4l2.la \
	$(NULL)

if BUILD_STATIC
V4L2_DECODE_LDFLAGS = \
	-Wl,--whole-archive \
	-Wl,$(top_builddir)/decoder/.libs/libyami_decoder.a \
	-Wl,--no-whole-archive
endif

if ENABLE_V4L2_GLX
V4L2_DECODE_LIBS += $(LIBGL_LIBS)
else
V4L2_DECODE_LIBS += $(LIBEGL_LIBS) $(LIBGLES2_LIBS)
endif

V4L2_ENCODE_LIBS = \
	$(YAMI_ENCODE_LIBS) \
	$(top_builddir)/v4l2/libyami_v4l2.la \
	$(NULL)

if BUILD_STATIC
V4L2_ENCODE_LDFLAGS = \
	-Wl,--whole-archive \
	-Wl,$(top_builddir)/encoder/.libs/libyami_encoder.a \
	-Wl,--no-whole-archive
endif

CAPI_DECODE_LIBS = \
	$(YAMI_DECODE_LIBS) \
	$(top_builddir)/capi/libyami_capi.la \
	$(NULL)

CAPI_ENCODE_LIBS = \
	$(YAMI_ENCODE_LIBS) \
	$(top_builddir)/capi/libyami_capi.la \
	$(NULL)

YAMI_VPP_LIBS = \
	$(YAMI_COMMON_LIBS) \
	$(top_builddir)/vpp/libyami_vpp.la \
	$(LIBYAMI_LT_LDFLAGS) \
	$(LIBVA_LIBS) \
	$(LIBVA_DRM_LIBS) \
	$(YAMI_ENCODE_LIBS) \
	$(NULL)

if BUILD_STATIC
YAMI_VPP_LDFLAGS = \
	-Wl,--whole-archive \
	-Wl,$(top_builddir)/vpp/.libs/libyami_vpp.a \
	-Wl,--no-whole-archive
endif

YAMI_VPP_CFLAGS = \
	$(LIBVA_CFLAGS) \
	$(NULL)

if ENABLE_CAPI
decodecapi_LDADD    = $(CAPI_DECODE_LIBS)
decodecapi_SOURCES  = decodecapi.c decodehelp.h  decodeInputCapi.cpp decodeOutputCapi.cpp $(DECODE_INPUT_SOURCES) decodeoutput.cpp
if ENABLE_TESTS_GLES
decodecapi_SOURCES += ../egl/egl_util.c ./egl/gles2_help.c
endif

encodecapi_LDADD    = $(CAPI_ENCODE_LIBS)
encodecapi_SOURCES  = encodecapi.c encodehelp.h encodeinput.cpp encodeInputCamera.cpp encodeInputDecoder.cpp encodeInputCapi.cpp $(DECODE_INPUT_SOURCES)
else
yamidecode_LDADD    = $(YAMI_DECODE_LIBS)
yamidecode_LDFLAGS  = $(YAMI_DECODE_LDFLAGS)
yamidecode_SOURCES  = decode.cpp decodehelp.h $(DECODE_INPUT_SOURCES) decodeoutput.cpp
if ENABLE_TESTS_GLES
yamidecode_SOURCES += ../egl/egl_util.c ./egl/gles2_help.c
endif

yamiencode_LDADD    = $(YAMI_ENCODE_LIBS)
yamiencode_LDFLAGS  = $(YAMI_ENCODE_LDFLAGS)
yamiencode_SOURCES  = encode.cpp encodeinput.cpp encodeInputCamera.cpp encodeInputDecoder.cpp $(DECODE_INPUT_SOURCES)

v4l2decode_LDADD   = $(V4L2_DECODE_LIBS)
v4l2decode_LDFLAGS = $(V4L2_DECODE_LDFLAGS)
v4l2decode_SOURCES = v4l2decode.cpp decodehelp.h $(DECODE_INPUT_SOURCES)
if ENABLE_V4L2_GLX
v4l2decode_SOURCES += ./glx/glx_help.c
else
v4l2decode_SOURCES += ./egl/gles2_help.c
v4l2decode_LDADD += -ldl
endif

v4l2encode_LDADD   = $(V4L2_ENCODE_LIBS)
v4l2encode_LDFLAGS = $(V4L2_ENCODE_LDFLAGS)
v4l2encode_SOURCES = v4l2encode.cpp encodeinput.h encodeinput.cpp encodeInputCamera.cpp encodeInputDecoder.cpp $(DECODE_INPUT_SOURCES)
endif

yamivpp_LDADD    = $(YAMI_VPP_LIBS)
yamivpp_LDFLAGS  = $(YAMI_VPP_LDFLAGS)
yamivpp_SOURCES  = vppinputdecode.cpp vppinputoutput.cpp vppoutputencode.cpp  vpp.cpp encodeinput.cpp encodeInputCamera.cpp encodeInputDecoder.cpp $(DECODE_INPUT_SOURCES)

yamitranscode_LDADD    = $(YAMI_VPP_LIBS)
yamitranscode_LDFLAGS  = -pthread $(YAMI_VPP_LDFLAGS)
yamitranscode_SOURCES  = vppinputdecode.cpp vppinputoutput.cpp vppoutputencode.cpp  yamitranscode.cpp encodeinput.cpp encodeInputCamera.cpp encodeInputDecoder.cpp $(DECODE_INPUT_SOURCES) vppinputasync.cpp 

bin_PROGRAMS += yamiinfo
yamiinfo_SOURCES = yamiinfo.cpp
yamiinfo_LDADD = \
	$(top_builddir)/decoder/libyami_decoder.la \
	$(top_builddir)/encoder/libyami_encoder.la \
	$(top_builddir)/vpp/libyami_vpp.la \
	$(NULL)
yamiinfo_LDFLAGS = -Wl,--no-as-needed \
	$(YAMI_DECODE_LDFLAGS) \
	$(YAMI_ENCODE_LDFLAGS) \
	$(YAMI_VPP_LDFLAGS) \
	$(NULL)
