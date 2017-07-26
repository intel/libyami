libyami_capi_source_c = \
	VideoDecoderCapi.cpp \
	VideoEncoderCapi.cpp \
	$(NULL)

libyami_capi_source_h = \
	../interface/VideoDecoderCapi.h \
	../interface/VideoEncoderCapi.h \
	$(NULL)

libyami_capi_source_h_priv = \
	$(NULL)

libyami_capi_ldflags = \
	$(LIBYAMI_LT_LDFLAGS) \
	$(NULL)

#to compile within yocto
extra_includes = \
        -I$(top_srcdir) \
        $(NULL)

libyami_capi_cppflags = \
	$(LIBVA_CFLAGS) \
	-I$(top_srcdir)/interface \
	$(extra_includes) \
	$(NULL)

noinst_LTLIBRARIES          = libyami_capi.la
libyami_capiincludedir      = $(includedir)/libyami
libyami_capiinclude_HEADERS = $(libyami_capi_source_h)
noinst_HEADERS              = $(libyami_capi_source_h_priv)
libyami_capi_la_SOURCES     = $(libyami_capi_source_c)
libyami_capi_la_LDFLAGS     = $(libyami_capi_ldflags) $(AM_LDFLAGS)
libyami_capi_la_CPPFLAGS    = $(libyami_capi_cppflags) $(AM_CPPFLAGS)

if ENABLE_TESTS
include Makefile.unittest
endif

DISTCLEANFILES = \
	Makefile.in


