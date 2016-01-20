INCLUDES = -I$(top_srcdir)

libyami_codecparser_source_c = \
	bitreader.c \
	bytereader.c \
	bytewriter.c \
	h264parser.c \
	h265parser.c \
	mpegvideoparser.c \
	mpeg4parser.c \
	vc1parser.c \
	vp8utils.c \
	vp8rangedecoder.c \
	vp8parser.c \
	vp9quant.c \
	vp9parser.c\
	dboolhuff.c \
	jpegparser.c \
	parserutils.c \
	nalutils.c \
	bitwriter.c \
	$(NULL)

libyami_codecparser_source_h_priv = \
	bitreader.h \
	bytereader.h \
	bytewriter.h \
	h264parser.h \
	h265parser.h \
	mpegvideoparser.h \
	mpeg4parser.h \
	vc1parser.h \
	vp8utils.h \
	vp8rangedecoder.h \
	vp8parser.h \
	vp9quant.h \
	vp9parser.h \
	jpegparser.h \
	parserutils.h \
	nalutils.h \
	bitwriter.h \
	$(NULL)

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

DISTCLEANFILES = \
	Makefile.in


