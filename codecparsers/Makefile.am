libyami_codecparser_source_c = \
	bitReader.cpp \
	bitWriter.cpp \
	nalReader.cpp \
	dboolhuff.c \
	$(NULL)

if BUILD_JPEG_PARSER
libyami_codecparser_source_c += \
	jpegParser.cpp \
	$(NULL)
endif

if BUILD_H264_DECODER
libyami_codecparser_source_c += \
	h264Parser.cpp \
	$(NULL)
endif

if BUILD_MPEG2_DECODER
libyami_codecparser_source_c += \
	mpeg2_parser.cpp \
	$(NULL)
endif

if BUILD_VP8_DECODER
libyami_codecparser_source_c += \
	vp8_bool_decoder.cpp \
	vp8_parser.cpp \
	$(NULL)
endif

if BUILD_H265_DECODER
libyami_codecparser_source_c += \
	h265Parser.cpp \
	$(NULL)
endif

if BUILD_VC1_DECODER
libyami_codecparser_source_c += \
	vc1Parser.cpp \
	$(NULL)
endif

if BUILD_VP9_DECODER
libyami_codecparser_source_c += \
	vp9quant.c \
	vp9parser.cpp \
	$(NULL)
endif

libyami_codecparser_source_h_priv = \
	bitReader.h \
	bitWriter.h \
	nalReader.h \
	$(NULL)

if BUILD_JPEG_PARSER
libyami_codecparser_source_h_priv += \
	jpegParser.h \
	$(NULL)
endif

if BUILD_H264_DECODER
libyami_codecparser_source_h_priv += \
	h264Parser.h \
	$(NULL)
endif

if BUILD_MPEG2_DECODER
libyami_codecparser_source_h_priv += \
	mpeg2_parser.h \
	$(NULL)
endif

if BUILD_VP8_DECODER
libyami_codecparser_source_h_priv += \
	vp8_bool_decoder.h \
	vp8_parser.h \
	$(NULL)
endif

libyami_codecparser_source_h_priv += \
	dboolhuff.h \
	$(NULL)
EXTRA_DIST = dboolhuff.LICENSE dboolhuff.PATENTS dboolhuff.AUTHORS

if BUILD_H265_DECODER
libyami_codecparser_source_h_priv += \
	h265Parser.h \
	$(NULL)
endif

if BUILD_VC1_DECODER
libyami_codecparser_source_h_priv += \
	vc1Parser.h \
	$(NULL)
endif

if BUILD_VP9_DECODER
libyami_codecparser_source_h_priv += \
	vp9quant.h \
	vp9parser.h \
	$(NULL)
endif

libyami_codecparser_ldflags = \
	$(LIBYAMI_LT_LDFLAGS) \
	$(NULL)

#to compile within yocto
extra_includes = \
        -I$(top_srcdir) \
        $(NULL)

libyami_codecparser_cppflags = \
	-Dvp8_norm=libyami_vp8_norm \
	-Dvp8dx_start_decode=libyami_vp8dx_start_decode \
	-Dvp8dx_bool_decoder_fill=libyami_vp8dx_bool_decoder_fill \
	-I$(top_srcdir)/interface \
	$(extra_includes) \
	$(NULL)

noinst_LTLIBRARIES                 = libyami_codecparser.la
libyami_codecparserincludedir      = ${includedir}/libyami
libyami_codecparserinclude_HEADERS = $(libyami_codecparser_source_h)
noinst_HEADERS                     = $(libyami_codecparser_source_h_priv)
libyami_codecparser_la_SOURCES     = $(libyami_codecparser_source_c)
libyami_codecparser_la_LDFLAGS     = $(libyami_codecparser_ldflags) $(AM_LDFLAGS)
libyami_codecparser_la_CPPFLAGS    = $(libyami_codecparser_cppflags) $(AM_CPPFLAGS)

if ENABLE_TESTS
include Makefile.unittest
endif

DISTCLEANFILES = \
	Makefile.in


