INCLUDES = -I$(top_srcdir)

if ENABLE_CAPI
bin_PROGRAMS = decodecapi encodecapi
else
bin_PROGRAMS = yamidecode yamiencode
if ENABLE_V4L2
bin_PROGRAMS += v4l2encode v4l2decode
endif
endif

AM_CFLAGS = \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/interface	\
	$(LIBVA_CFLAGS)             \
	$(NULL)

AM_CPPFLAGS = $(AM_CFLAGS)

YAMI_COMMON_LIBS 	= \
	$(top_builddir)/common/libyami_common.la	\
	$(top_builddir)/vaapi/libyami_vaapi.la		\
	$(NULL)

YAMI_DECODE_LIBS 	= \
	$(YAMI_COMMON_LIBS)                            	\
	$(top_builddir)/decoder/libyami_decoder.la      \
	$(NULL)
if ENABLE_TESTS_GLES
YAMI_DECODE_LIBS += $(LIBEGL_LIBS) $(LIBGLES2_LIBS)
endif

YAMI_ENCODE_LIBS = \
	$(YAMI_COMMON_LIBS)                             \
	$(top_builddir)/encoder/libyami_encoder.la      \
	$(NULL)

V4L2_DECODE_LIBS = \
	$(YAMI_DECODE_LIBS)                         \
	$(top_builddir)/v4l2/libyami_v4l2.la        \
	$(LIBEGL_LIBS)                              \
	$(LIBGLES2_LIBS)                            \
	$(NULL)

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


if ENABLE_CAPI
decodecapi_LDADD	= $(CAPI_ENCODE_LIBS) -lX11
decodecapi_SOURCES	= decodecapi.c decodehelp.h decodeinput.cpp  decodeoutput.cpp decodeInputCapi.cpp decodeOutputCapi.cpp
if ENABLE_TESTS_GLES
decodecapi_SOURCES += ../egl/egl_util.c ./egl/gles2_help.c
endif

encodecapi_LDADD	= $(CAPI_ENCODE_LIBS) -lX11
encodecapi_SOURCES	= encodecapi.c encodehelp.h encodeinput.cpp encodeInputCamera.cpp encodeInputCapi.cpp
else
yamidecode_LDADD	= $(YAMI_DECODE_LIBS) -lX11
yamidecode_SOURCES	= decode.cpp decodehelp.h decodeinput.cpp decodeoutput.cpp
if ENABLE_TESTS_GLES
yamidecode_SOURCES += ../egl/egl_util.c ./egl/gles2_help.c
endif

yamiencode_LDADD    = $(YAMI_ENCODE_LIBS) -lX11
yamiencode_SOURCES  = encode.cpp encodeinput.cpp encodeInputCamera.cpp

v4l2decode_LDADD = $(V4L2_DECODE_LIBS) -lX11
v4l2decode_SOURCES = v4l2decode.cpp decodehelp.h decodeinput.cpp ../egl/egl_util.c ./egl/gles2_help.c

v4l2encode_LDADD = $(V4L2_ENCODE_LIBS)
v4l2encode_SOURCES = v4l2encode.cpp encodeinput.h encodeinput.cpp encodeInputCamera.cpp
endif
