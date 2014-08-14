bin_PROGRAMS = decode h264encode

AM_CPPFLAGS = \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/interface	\
	$(LIBVA_CFLAGS)             \
	$(NULL)

YAMI_COMMON_LIBS 	= \
	$(top_builddir)/common/libyami_common.la		\
	$(top_builddir)/vaapi/libyami_vaapi.la		\
	$(NULL)

YAMI_DECODE_LIBS 	= \
	$(YAMI_COMMON_LIBS)                             \
	$(top_builddir)/decoder/libyami_decoder.la		\
	$(NULL)

YAMI_ENCODE_LIBS = \
	$(YAMI_COMMON_LIBS)                             \
	$(top_builddir)/encoder/libyami_encoder.la		\
	$(NULL)

decode_LDADD	= $(YAMI_DECODE_LIBS) -lX11
decode_SOURCES	= decode.cpp

h264encode_LDADD	= $(YAMI_ENCODE_LIBS) -lX11
h264encode_SOURCES	= h264encode.cpp
