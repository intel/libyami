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

libyami_capi_la_LIBADD = \
		$(top_builddir)/decoder/libyami_decoder.la \
		$(top_builddir)/encoder/libyami_encoder.la \
		$(NULL)

libyami_capi_ldflags = \
        $(LIBYAMI_LT_LDFLAGS) \
        $(NULL)

libyami_capi_cppflags = \
        $(LIBVA_CFLAGS) \
        $(NULL)


lib_LTLIBRARIES	               = libyami_capi.la
libyami_capiincludedir      = $(includedir)/libyami_capi
libyami_capiinclude_HEADERS = $(libyami_capi_source_h)
noinst_HEADERS                 = $(libyami_capi_source_h_priv)
libyami_capi_la_SOURCES     = $(libyami_capi_source_c)
libyami_capi_la_LDFLAGS     = $(libyami_capi_ldflags)
libyami_capi_la_CPPFLAGS    = $(libyami_capi_cppflags)

DISTCLEANFILES = \
	Makefile.in


