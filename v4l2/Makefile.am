INCLUDES = -I$(top_srcdir) \
           -I$(top_srcdir)/interface \
		   $(NULL)

libyami_v4l2_source_c = \
        v4l2_codecbase.cpp \
        ../egl/egl_util.c \
        v4l2_encode.cpp \
        v4l2_wrapper.cpp \
        $(NULL)

libyami_v4l2_source_h = \
		v4l2_wrapper.h \
        $(NULL)

libyami_v4l2_source_h_priv = \
        v4l2_codecbase.h \
        v4l2_encode.h \
        $(NULL)

libyami_v4l2_la_LIBADD = \
		$(top_builddir)/decoder/libyami_decoder.la \
		$(top_builddir)/encoder/libyami_encoder.la \
		$(NULL)

libyami_v4l2_ldflags = \
        $(LIBYAMI_LT_LDFLAGS) \
        $(LIBEGL_LIBS)        \
        -lpthread             \
        $(NULL)

libyami_v4l2_cppflags = \
        $(LIBVA_CFLAGS) \
        $(LIBV4L2_CFLAGS) \
        $(LIBEGL_CFLAGS)  \
        $(NULL)
if ENABLE_DMABUF
libyami_v4l2_cppflags += $(LIBDRM_CFLAGS)
endif

lib_LTLIBRARIES	               = libyami_v4l2.la
libyami_v4l2includedir      = $(includedir)/libyami_v4l2
libyami_v4l2include_HEADERS = $(libyami_v4l2_source_h)
noinst_HEADERS                 = $(libyami_v4l2_source_h_priv)
libyami_v4l2_la_SOURCES     = $(libyami_v4l2_source_c)
libyami_v4l2_la_LDFLAGS     = $(libyami_v4l2_ldflags)
libyami_v4l2_la_CPPFLAGS    = $(libyami_v4l2_cppflags)

DISTCLEANFILES = \
	Makefile.in


