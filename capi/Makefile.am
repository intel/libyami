INCLUDES = -I$(top_srcdir) \
	-I$(top_srcdir)/interface \
	$(NULL)

libyami_capi_source_c = \
	VideoDecoderCapi.cpp \
	VideoEncoderCapi.cpp \
	$(NULL)

libyami_capi_source_h = \
	VideoDecoderCapi.h \
	VideoEncoderCapi.h \
	$(NULL)

libyami_capi_source_h_priv = \
	$(NULL)

libyami_capi_ldflags = \
	$(LIBYAMI_LT_LDFLAGS) \
	$(NULL)

libyami_capi_cppflags = \
	$(LIBVA_CFLAGS) \
	$(NULL)

noinst_LTLIBRARIES          = libyami_capi.la
libyami_capiincludedir      = $(includedir)/libyami
libyami_capiinclude_HEADERS = $(libyami_capi_source_h)
noinst_HEADERS              = $(libyami_capi_source_h_priv)
libyami_capi_la_SOURCES     = $(libyami_capi_source_c)
libyami_capi_la_LDFLAGS     = $(libyami_capi_ldflags)
libyami_capi_la_CPPFLAGS    = $(libyami_capi_cppflags)

if ENABLE_UNITTESTS
noinst_PROGRAMS = unittest

unittest_SOURCES = \
	$(top_builddir)/common/unittest_main.cpp \
	$(NULL)

unittest_LDFLAGS = \
	$(GTEST_LDFLAGS) \
	$(NULL)

unittest_LDADD = \
	libyami_capi.la \
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


