INCLUDES = -I$(top_srcdir)

libyami_codecparser_source_c = \
	vp9quant.c \
	dboolhuff.c \
	$(NULL)

libyami_codecparser_source_h_priv = \
	vp9quant.h \
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

if ENABLE_UNITTESTS
noinst_PROGRAMS = unittest

unittest_SOURCES = \
	$(top_builddir)/common/unittest_main.cpp \
	$(NULL)

unittest_LDFLAGS = \
	$(GTEST_LDFLAGS) \
	$(NULL)

unittest_LDADD = \
	libyami_codecparser.la \
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


