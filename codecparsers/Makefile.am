INCLUDES = -I$(top_srcdir)

libyami_codecparser_source_c = \
	bitReader.cpp \
	nalReader.cpp \
	vp9quant.c \
	dboolhuff.c \
	$(NULL)

if BUILD_JPEG_DECODER
libyami_codecparser_source_c += \
	jpegParser.cpp \
	$(NULL)
endif

if BUILD_H264_DECODER
libyami_codecparser_source_c += \
	h264Parser.cpp \
	$(NULL)
endif

if BUILD_VP8_DECODER
libyami_codecparser_source_c += \
	vp8_bool_decoder.cpp \
	vp8_parser.cpp \
	$(NULL)
endif

libyami_codecparser_source_h_priv = \
	bitReader.h \
	nalReader.h \
	vp9quant.h \
	$(NULL)

if BUILD_JPEG_DECODER
libyami_codecparser_source_h_priv += \
	jpegParser.h \
	$(NULL)
endif

if BUILD_H264_DECODER
libyami_codecparser_source_h_priv += \
	h264Parser.h \
	$(NULL)
endif

if BUILD_VP8_DECODER
libyami_codecparser_source_h_priv += \
	vp8_bool_decoder.h \
	vp8_parser.h \
	$(NULL)
endif

libyami_codecparser_source_h_priv = \
	dboolhuff.h \
	$(NULL)
EXTRA_DIST = dboolhuff.LICENSE dboolhuff.PATENTS dboolhuff.AUTHORS

libyami_codecparser_ldflags = \
	$(LIBYAMI_LT_LDFLAGS) \
	$(NULL)

libyami_codecparser_cppflags = \
	-Dvp8_norm=libyami_vp8_norm \
	-Dvp8dx_start_decode=libyami_vp8dx_start_decode \
	-Dvp8dx_bool_decoder_fill=libyami_vp8dx_bool_decoder_fill \
	$(NULL)

noinst_LTLIBRARIES                 = libyami_codecparser.la
libyami_codecparserincludedir      = ${includedir}/libyami
libyami_codecparserinclude_HEADERS = $(libyami_codecparser_source_h)
noinst_HEADERS                     = $(libyami_codecparser_source_h_priv)
libyami_codecparser_la_SOURCES     = $(libyami_codecparser_source_c)
libyami_codecparser_la_LDFLAGS     = $(libyami_codecparser_ldflags)
libyami_codecparser_la_CPPFLAGS    = $(libyami_codecparser_cppflags)

if ENABLE_UNITTESTS
noinst_PROGRAMS = unittest

unittest_SOURCES = \
	$(top_builddir)/common/unittest_main.cpp \
	bitReader_unittest.cpp \
	$(NULL)

if BUILD_VP8_DECODER
unittest_SOURCES += \
        vp8_bool_decoder_unittest.cpp \
        $(NULL)
endif

if BUILD_JPEG_DECODER
unittest_SOURCES += \
        jpegParser_unittest.cpp \
        $(NULL)
endif

if BUILD_H264_DECODER
unittest_SOURCES += \
        h264Parser_unittest.cpp \
        $(NULL)
endif

unittest_LDFLAGS = \
	$(GTEST_LDFLAGS) \
	$(NULL)

unittest_LDADD = \
	libyami_codecparser.la \
	$(top_builddir)/common/libyami_common.la \
	$(GTEST_LIBS) \
	$(NULL)

unittest_CPPFLAGS = \
	$(GTEST_CPPFLAGS) \
	$(NULL)

unittest_CXXFLAGS = \
	$(GTEST_CXXFLAGS) \
	$(NULL)

check-local: unittest
	$(builddir)/unittest
endif

DISTCLEANFILES = \
	Makefile.in


