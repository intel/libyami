libyamiv4l2_source_c = \
	v4l2_codecbase.cpp \
	v4l2_decode.cpp \
	v4l2_encode.cpp \
	v4l2_wrapper.cpp \
	$(NULL)

if ENABLE_EGL
libyamiv4l2_source_c += ../egl/egl_util.c ../egl/egl_vaapi_image.cpp
endif

libyamiv4l2_source_h = \
	../interface/v4l2_wrapper.h \
	../interface/v4l2codec_device_ops.h \
	$(NULL)

libyamiv4l2_source_h_priv = \
	v4l2_codecbase.h \
	v4l2_encode.h \
	v4l2_decode.h \
	$(NULL)

libyamiv4l2_ldflags = \
	$(LIBYAMI_LT_LDFLAGS) \
	-pthread \
	$(NULL)

if ENABLE_EGL
libyamiv4l2_ldflags += $(LIBEGL_LIBS)
endif

#to compile within yocto
extra_includes = \
        -I$(top_srcdir) \
        $(NULL)

libyamiv4l2_cppflags = \
	$(LIBVA_CFLAGS) \
	$(LIBV4L2_CFLAGS) \
	-I$(top_srcdir)/interface \
	$(extra_includes) \
	$(NULL)

if ENABLE_EGL
libyamiv4l2_cppflags += $(LIBEGL_CFLAGS)
endif

if ENABLE_DMABUF
libyamiv4l2_cppflags += $(LIBDRM_CFLAGS)
endif


noinst_LTLIBRARIES         = libyamiv4l2.la
libyamiv4l2includedir      = $(includedir)/libyami
libyamiv4l2include_HEADERS = $(libyamiv4l2_source_h)
noinst_HEADERS             = $(libyamiv4l2_source_h_priv)
libyamiv4l2_la_SOURCES     = $(libyamiv4l2_source_c)
libyamiv4l2_la_LDFLAGS     = $(libyamiv4l2_ldflags) $(AM_LDFLAGS)
libyamiv4l2_la_CPPFLAGS    = $(libyamiv4l2_cppflags) $(AM_CPPFLAGS)

if ENABLE_TESTS
-include Makefile.unittest
endif

DISTCLEANFILES = \
	Makefile.in


