libcodecparser_source_c = \
        bitreader.c \
        bytereader.c \
        h264parser.c \
        mpegvideoparser.c \
        mpeg4parser.c \
        vc1parser.c \
        dboolhuff.c \
        vp8parser.c \
        jpegparser.c \
        parserutils.c \
        nalreader.c \
        bitwriter.c \
	$(NULL)

libcodecparser_source_h = \
        bitreader.h \
        bytereader.h \
        h264parser.h \
        mpegvideoparser.h \
        mpeg4parser.h \
        vc1parser.h \
        vp8parser.h \
        jpegparser.h \
        parserutils.h \
        nalreader.h \
        bitwriter.h \
        log.h \
	$(NULL)

libcodecparser_source_h_priv = \
		dboolhuff.h \
	$(NULL)

EXTRA_DIST = dboolhuff.LICENSE dboolhuff.PATENTS dboolhuff.AUTHORS

libcodecparser_ldflags = \
	-version-info 0:2:0  \
	$(NULL)

libcodecparser_cppflags = \
   -Dvp8_norm=libyami_vp8_norm \
   -Dvp8dx_start_decode=libyami_vp8dx_start_decode \
   -Dvp8dx_bool_decoder_fill=libyami_vp8dx_bool_decoder_fill
	$(NULL)

lib_LTLIBRARIES			= libcodecparser.la
libcodecparserincludedir         = ${includedir}/libcodecparser
libcodecparserinclude_HEADERS   = $(libcodecparser_source_h)
noinst_HEADERS                  = $(libcodecparser_source_h_priv)
libcodecparser_la_SOURCES	= $(libcodecparser_source_c)
libcodecparser_la_LDFLAGS	= $(libcodecparser_ldflags)
libcodecparser_la_CPPFLAGS      = $(libcodecparser_cppflags)

DISTCLEANFILES = \
	Makefile.in


