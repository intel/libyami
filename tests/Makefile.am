INCLUDES = -I$(top_srcdir)

if ENABLE_CAPI
bin_PROGRAMS = decodecapi encodecapi
else
bin_PROGRAMS = yamidecode yamiencode yamivpp
if ENABLE_V4L2
bin_PROGRAMS += v4l2encode v4l2decode
endif
if ENABLE_X11
bin_PROGRAMS += simpleplayer
endif
endif

AM_CFLAGS = \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/interface	\
	$(LIBVA_CFLAGS)             \
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

YAMI_COMMON_LIBS 	= \
	$(top_builddir)/common/libyami_common.la	\
	$(top_builddir)/vaapi/libyami_vaapi.la		\
	$(NULL)
if ENABLE_X11
YAMI_COMMON_LIBS += $(LIBVA_X11_LIBS) -lX11
endif

YAMI_DECODE_LIBS 	= \
	$(YAMI_COMMON_LIBS)                            	\
	$(top_builddir)/decoder/libyami_decoder.la      \
	$(NULL)
if ENABLE_TESTS_GLES
YAMI_DECODE_LIBS += $(LIBEGL_LIBS) $(LIBGLES2_LIBS)
endif
if ENABLE_AVFORMAT
YAMI_DECODE_LIBS += $(LIBAVFORMAT_LIBS)
endif

YAMI_ENCODE_LIBS = \
	$(YAMI_COMMON_LIBS)                             \
	$(top_builddir)/encoder/libyami_encoder.la      \
	$(YAMI_DECODE_LIBS)								\
	$(NULL)

V4L2_DECODE_LIBS = \
	$(YAMI_DECODE_LIBS)                         \
	$(top_builddir)/v4l2/libyami_v4l2.la        \
	$(NULL)
if ENABLE_V4L2_GLX
V4L2_DECODE_LIBS += $(LIBGL_LIBS)
else
V4L2_DECODE_LIBS += $(LIBEGL_LIBS) $(LIBGLES2_LIBS)
endif

V4L2_ENCODE_LIBS = \
	$(YAMI_ENCODE_LIBS)                         \
	$(top_builddir)/v4l2/libyami_v4l2.la        \
	$(NULL)

CAPI_DECODE_LIBS = \
	$(YAMI_DECODE_LIBS)							\
	$(top_builddir)/capi/libyami_capi.la		\
	$(NULL)

CAPI_ENCODE_LIBS = \
	$(YAMI_ENCODE_LIBS)							\
	$(top_builddir)/capi/libyami_capi.la		\
	$(NULL)

YAMI_VPP_LIBS = \
    $(YAMI_COMMON_LIBS)   \
    $(top_builddir)/vpp/libyami_vpp.la	\
    $(LIBYAMI_LT_LDFLAGS) \
    $(LIBVA_LIBS) \
    $(LIBVA_DRM_LIBS) \
    $(YAMI_ENCODE_LIBS) \
	$(NULL)

YAMI_VPP_CFLAGS = \
    $(LIBVA_CFLAGS) \
    -fpermissive \
	$(NULL)

if ENABLE_CAPI
decodecapi_LDADD	= $(CAPI_DECODE_LIBS)
decodecapi_SOURCES	= decodecapi.c decodehelp.h  decodeInputCapi.cpp decodeOutputCapi.cpp $(DECODE_INPUT_SOURCES) decodeoutput.cpp
if ENABLE_TESTS_GLES
decodecapi_SOURCES += ../egl/egl_util.c ./egl/gles2_help.c
endif

encodecapi_LDADD	= $(CAPI_ENCODE_LIBS)
encodecapi_SOURCES	= encodecapi.c encodehelp.h encodeinput.cpp encodeInputCamera.cpp encodeInputDecoder.cpp encodeInputCapi.cpp $(DECODE_INPUT_SOURCES)
else
yamidecode_LDADD	= $(YAMI_DECODE_LIBS)
yamidecode_SOURCES	= decode.cpp decodehelp.h $(DECODE_INPUT_SOURCES) decodeoutput.cpp
if ENABLE_TESTS_GLES
yamidecode_SOURCES += ../egl/egl_util.c ./egl/gles2_help.c
endif

yamiencode_LDADD    = $(YAMI_ENCODE_LIBS)
yamiencode_SOURCES  = encode.cpp encodeinput.cpp encodeInputCamera.cpp encodeInputDecoder.cpp $(DECODE_INPUT_SOURCES)

v4l2decode_LDADD = $(V4L2_DECODE_LIBS)
v4l2decode_SOURCES = v4l2decode.cpp decodehelp.h $(DECODE_INPUT_SOURCES)
if ENABLE_V4L2_GLX
v4l2decode_SOURCES += ./glx/glx_help.c
else
v4l2decode_SOURCES += ../egl/egl_util.c ./egl/gles2_help.c
endif

v4l2encode_LDADD = $(V4L2_ENCODE_LIBS)
v4l2encode_SOURCES = v4l2encode.cpp encodeinput.h encodeinput.cpp encodeInputCamera.cpp encodeInputDecoder.cpp $(DECODE_INPUT_SOURCES)
endif

yamivpp_LDADD    = $(YAMI_VPP_LIBS)
yamivpp_SOURCES  = vppinputoutput.cpp vppoutputencode.cpp  vpp.cpp encodeinput.cpp encodeInputCamera.cpp encodeInputDecoder.cpp $(DECODE_INPUT_SOURCES)

if ENABLE_X11
simpleplayer_LDADD = $(YAMI_DECODE_LIBS)
simpleplayer_SOURCES	= simpleplayer.cpp $(DECODE_INPUT_SOURCES)
endif
