INCLUDES = -I$(top_srcdir) \
	-I$(top_srcdir)/interface \
	$(NULL)

libyami_v4l2_source_c = \
	v4l2_codecbase.cpp \
	v4l2_decode.cpp \
	v4l2_encode.cpp \
	v4l2_wrapper.cpp \
	$(NULL)

libyami_v4l2_source_c += ../egl/egl_util.c ../egl/egl_vaapi_image.cpp

libyami_v4l2_source_h = \
	v4l2_wrapper.h \
	$(NULL)

libyami_v4l2_source_h_priv = \
	v4l2_codecbase.h \
	v4l2_encode.h \
	v4l2_decode.h \
	$(NULL)

libyami_v4l2_la_LIBADD = \
	$(top_builddir)/libyami.la \
	$(NULL)

libyami_v4l2_ldflags = \
	$(LIBYAMI_LT_LDFLAGS) \
	-pthread \
	$(NULL)

libyami_v4l2_ldflags += $(LIBEGL_LIBS)

libyami_v4l2_cppflags = \
	$(LIBVA_CFLAGS) \
	$(LIBV4L2_CFLAGS) \
	$(NULL)

if ENABLE_DMABUF
libyami_v4l2_cppflags += $(LIBDRM_CFLAGS)
endif

libyami_v4l2_cppflags += $(LIBEGL_CFLAGS)

lib_LTLIBRARIES             = libyami_v4l2.la
libyami_v4l2includedir      = $(includedir)/libyami
libyami_v4l2include_HEADERS = $(libyami_v4l2_source_h)
noinst_HEADERS              = $(libyami_v4l2_source_h_priv)
libyami_v4l2_la_SOURCES     = $(libyami_v4l2_source_c)
libyami_v4l2_la_LDFLAGS     = $(libyami_v4l2_ldflags)
libyami_v4l2_la_CPPFLAGS    = $(libyami_v4l2_cppflags)

if ENABLE_UNITTESTS
noinst_PROGRAMS = unittest

unittest_SOURCES = \
	$(top_builddir)/common/unittest_main.cpp \
	$(NULL)

unittest_LDFLAGS = \
	$(GTEST_LDFLAGS) \
	$(NULL)

unittest_LDADD = \
	libyami_v4l2.la \
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


