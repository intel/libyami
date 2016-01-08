INCLUDES = -I$(top_srcdir)

libyami_common_source_c = \
	log.cpp \
	utils.cpp \
	nalreader.cpp \
	surfacepool.cpp \
	YamiVersion.cpp \
	$(NULL)

libyami_common_source_h = \
	YamiVersion.h \
	$(NULL)

libyami_common_source_h_priv = \
	NonCopyable.h \
	Functional.h \
	log.h \
	utils.h \
	common_def.h \
	nalreader.h \
	videopool.h \
	surfacepool.h \
	$(NULL)

libyami_common_ldflags = \
	$(LIBYAMI_LT_LDFLAGS) \
	$(LIBVA_LIBS) \
	$(LIBVA_DRM_LIBS) \
	$(NULL)

libyami_common_cppflags = \
	$(LIBVA_CFLAGS) \
	$(NULL)

noinst_LTLIBRARIES            = libyami_common.la
libyami_commonincludedir      = ${includedir}/libyami
libyami_commoninclude_HEADERS = $(libyami_common_source_h)
libyami_common_la_SOURCES     = $(libyami_common_source_c)
libyami_common_la_LDFLAGS     = $(libyami_common_ldflags)
libyami_common_la_CPPFLAGS    = $(libyami_common_cppflags)

if ENABLE_UNITTESTS
noinst_PROGRAMS = unittest

unittest_SOURCES = \
	$(top_builddir)/common/unittest_main.cpp \
	factory_unittest.cpp \
	nalreader_unittest.cpp \
	utils_unittest.cpp \
	$(NULL)

unittest_LDFLAGS = \
	$(GTEST_LDFLAGS) \
	$(NULL)

unittest_LDADD = \
	libyami_common.la \
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
	Makefile.in\
	YamiVersion.h.in \
	$(NULL)


