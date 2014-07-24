bin_PROGRAMS = decode

AM_CPPFLAGS = \
	-I$(top_srcdir)				\
	-I$(top_srcdir)/interface	\
	$(NULL)

TEST_LIBS = \
	$(top_builddir)/decoder/libyami_decoder.la		\
	$(NULL)

decode_LDADD	= $(TEST_LIBS) -lX11
decode_SOURCES	= decode.cpp

