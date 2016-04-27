INCLUDES = -I$(top_srcdir) \
	-I$(top_srcdir)/interface \
	-I$(top_srcdir)/common \
	-I$(top_srcdir)/vaapi \
	-I$(top_srcdir)/codecparsers \
	$(NULL)

libyami_decoder_source_c = \
	vaapidecoder_base.cpp \
	vaapidecoder_host.cpp \
	vaapidecsurfacepool.cpp \
	vaapidecpicture.cpp \
	$(NULL)

if BUILD_VP8_DECODER
libyami_decoder_source_c += vaapidecoder_vp8.cpp
endif

if BUILD_VP9_DECODER
libyami_decoder_source_c += vaapidecoder_vp9.cpp
endif

if BUILD_FAKE_DECODER
libyami_decoder_source_c += vaapidecoder_fake.cpp
endif

libyami_decoder_source_h = \
	../interface/VideoCommonDefs.h \
	../interface/VideoDecoderDefs.h \
	../interface/VideoDecoderInterface.h \
	../interface/VideoDecoderHost.h \
	$(NULL)

libyami_decoder_source_h_priv = \
	vaapidecoder_base.h \
	vaapidecsurfacepool.h \
	vaapidecpicture.h \
	$(NULL)

if BUILD_VP8_DECODER
libyami_decoder_source_h_priv += vaapidecoder_vp8.h
endif

if BUILD_H265_DECODER
libyami_decoder_source_h_priv += vaapidecoder_h265.h
endif

if BUILD_VP9_DECODER
libyami_decoder_source_h_priv += vaapidecoder_vp9.h
endif

if BUILD_FAKE_DECODER
libyami_decoder_source_h_priv += vaapidecoder_fake.h
endif

libyami_decoder_ldflags = \
	$(LIBYAMI_LT_LDFLAGS) \
	$(LIBVA_LIBS) \
	$(LIBVA_DRM_LIBS) \
	-ldl \
	$(NULL)

if ENABLE_X11
libyami_decoder_ldflags += $(LIBVA_X11_LIBS) $(X11_LIBS)
endif

libyami_decoder_cppflags = \
	$(LIBVA_CFLAGS) \
	$(LIBVA_DRM_CFLAGS) \
	$(NULL)

if ENABLE_X11
libyami_decoder_cppflags += $(LIBVA_X11_CFLAGS)
endif

noinst_LTLIBRARIES             = libyami_decoder.la
libyami_decoderincludedir      = $(includedir)/libyami
libyami_decoderinclude_HEADERS = $(libyami_decoder_source_h)
noinst_HEADERS                 = $(libyami_decoder_source_h_priv)
libyami_decoder_la_SOURCES     = $(libyami_decoder_source_c)
libyami_decoder_la_LDFLAGS     = $(libyami_decoder_ldflags)
libyami_decoder_la_CPPFLAGS    = $(libyami_decoder_cppflags)

if ENABLE_UNITTESTS
noinst_PROGRAMS = unittest

unittest_SOURCES = \
	$(top_builddir)/common/unittest_main.cpp \
	$(NULL)

if BUILD_VP8_DECODER
unittest_SOURCES += vaapidecoder_vp8_unittest.cpp
endif

unittest_LDFLAGS = \
	$(GTEST_LDFLAGS) \
	$(NULL)

unittest_LDADD = \
	libyami_decoder.la \
        $(top_builddir)/codecparsers/libyami_codecparser.la \
        $(top_builddir)/vaapi/libyami_vaapi.la \
        $(top_builddir)/common/libyami_common.la \
	$(GTEST_LIBS) \
	$(NULL)

unittest_CPPFLAGS = \
	$(GTEST_CPPFLAGS) \
	$(LIBVA_CFLAGS) \
	$(NULL)

unittest_CXXFLAGS = \
	$(GTEST_CXXFLAGS) \
	$(NULL)

check-local: unittest
	$(builddir)/unittest
endif

DISTCLEANFILES = \
	Makefile.in 


