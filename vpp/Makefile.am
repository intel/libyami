INCLUDES = -I$(top_srcdir) \
           -I$(top_srcdir)/interface \
           -I$(top_srcdir)/common \
           -I$(top_srcdir)/vaapi

libyami_vpp_source_c = \
        vaapipostprocess_base.cpp \
        vaapipostprocess_host.cpp \
        vaapipostprocess_scaler.cpp \
        vaapivpppicture.cpp \
        $(NULL)


libyami_vpp_source_h = \
        ../interface/VideoPostProcessHost.h           \
        ../interface/VideoPostProcessInterface.h      \
        $(NULL)

libyami_vpp_source_h_priv = \
        vaapipostprocess_base.h     \
        vaapipostprocess_scaler.h   \
        vaapivpppicture.h           \
        $(NULL)

libyami_vpp_la_LIBADD = \
        $(top_builddir)/common/libyami_common.la \
        $(top_builddir)/vaapi/libyami_vaapi.la

libyami_vpp_ldflags = \
        $(LIBYAMI_LT_LDFLAGS) \
        -ldl                 \
        $(NULL)

libyami_vpp_cppflags = \
        $(LIBVA_CFLAGS) \
        $(LIBVA_DRM_CFLAGS) \
        -fpermissive \
        $(NULL)

lib_LTLIBRARIES             = libyami_vpp.la
libyami_vppincludedir       = $(includedir)/libyami_vpp
libyami_vppinclude_HEADERS  = $(libyami_vpp_source_h)
noinst_HEADERS              = $(libyami_vpp_source_h_priv)
libyami_vpp_la_SOURCES      = $(libyami_vpp_source_c)
libyami_vpp_la_LDFLAGS      = $(libyami_vpp_ldflags)
libyami_vpp_la_CPPFLAGS     = $(libyami_vpp_cppflags)

DISTCLEANFILES = \
    Makefile.in
