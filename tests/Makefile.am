bin_PROGRAMS = decode h264encode
if ENABLE_V4L2
bin_PROGRAMS += v4l2encode
endif

if ENABLE_CAPI
bin_PROGRAMS += decodecapi
endif

AM_CPPFLAGS = \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/interface	\
	$(LIBVA_CFLAGS)             \
	$(NULL)

YAMI_COMMON_LIBS 	= \
	$(top_builddir)/common/libyami_common.la	\
	$(top_builddir)/vaapi/libyami_vaapi.la		\
	$(NULL)

YAMI_DECODE_LIBS 	= \
	$(YAMI_COMMON_LIBS)                            	\
	$(top_builddir)/decoder/libyami_decoder.la      \
	$(NULL)

YAMI_ENCODE_LIBS = \
	$(YAMI_COMMON_LIBS)                             \
	$(top_builddir)/encoder/libyami_encoder.la      \
	$(NULL)

V4L2_ENCODE_LIBS = \
	$(YAMI_ENCODE_LIBS)                         \
	$(top_builddir)/v4l2/libyami_v4l2.la        \
	$(NULL)

decode_LDADD    = $(YAMI_DECODE_LIBS) -lX11
decode_SOURCES  = decode.cpp

h264encode_LDADD    = $(YAMI_ENCODE_LIBS) -lX11
h264encode_SOURCES  = h264encode.cpp encodehelp.h

v4l2encode_LDADD = $(V4L2_ENCODE_LIBS)
v4l2encode_SOURCES = v4l2encode.cpp encodehelp.h

if ENABLE_CAPI
YAMI_COMMON_LIBS += $(top_builddir)/capi/libyami_capi.la
decodecapi_LDADD	= $(YAMI_DECODE_LIBS) -lX11
decodecapi_SOURCES	= decodecapi.c
endif
