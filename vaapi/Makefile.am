INCLUDES = -I$(top_srcdir)

libyami_vaapi_source_c = \
	vaapipicture.cpp \
	VaapiBuffer.cpp \
	VaapiSurface.cpp\
	VaapiUtils.cpp \
	vaapidisplay.cpp \
	vaapicontext.cpp \
	vaapisurfaceallocator.cpp \
	$(NULL)

libyami_vaapi_source_h_priv = \
	vaapipicture.h \
	VaapiBuffer.h \
	VaapiSurface.h \
	VaapiUtils.h \
	vaapidisplay.h \
	vaapicontext.h \
	vaapisurfaceallocator.h \
	$(NULL)

libyami_vaapi_ldflags = \
	$(LIBYAMI_LT_LDFLAGS) \
	$(LIBVA_LIBS) \
	$(LIBVA_DRM_LIBS) \
	$(NULL)

if ENABLE_X11
libyami_vaapi_ldflags += $(LIBVA_X11_LIBS) $(X11_LIBS)
endif

libyami_vaapi_cppflags = \
	$(LIBVA_CFLAGS) \
	$(NULL)

noinst_LTLIBRARIES           = libyami_vaapi.la
libyami_vaapiincludedir      = ${includedir}/libyami
libyami_vaapiinclude_HEADERS = $(libyami_vaapi_source_h)
libyami_vaapi_la_SOURCES     = $(libyami_vaapi_source_c)
libyami_vaapi_la_LDFLAGS     = $(libyami_vaapi_ldflags)
libyami_vaapi_la_CPPFLAGS    = $(libyami_vaapi_cppflags)

if ENABLE_UNITTESTS
noinst_PROGRAMS = unittest

unittest_SOURCES = \
	$(top_builddir)/common/unittest_main.cpp \
	vaapidisplay_unittest.cpp \
	$(NULL)

unittest_LDFLAGS = \
	$(GTEST_LDFLAGS) \
	$(NULL)

unittest_LDADD = \
	libyami_vaapi.la \
	$(top_builddir)/common/libyami_common.la \
	$(GTEST_LIBS) \
	$(NULL)

if ENABLE_X11
unittest_LDADD += $(X11_LIBS)
endif

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


