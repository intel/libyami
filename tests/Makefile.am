bin_PROGRAMS = decode h264encode

AM_CPPFLAGS = \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/interface	\
	$(NULL)

DECODE_LIBS 	= \
	$(top_builddir)/decoder/libyami_decoder.la		\
	$(NULL)

H264ENCODE_LIBS = \
	$(top_builddir)/encoder/libyami_encoder.la		\
	$(NULL)

decode_LDADD	= $(DECODE_LIBS) -lX11
decode_SOURCES	= decode.cpp

h264encode_LDADD	= $(H264ENCODE_LIBS) -lX11
h264encode_SOURCES	= h264encode.cpp
