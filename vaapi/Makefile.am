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
	vaapistreamable.h \
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

#to compile within yocto
extra_includes = \
        -I$(top_srcdir) \
        $(NULL)

libyami_vaapi_cppflags = \
	$(LIBVA_CFLAGS) \
	-I$(top_srcdir)/interface \
	$(extra_includes) \
	$(NULL)

noinst_LTLIBRARIES           = libyami_vaapi.la
libyami_vaapiincludedir      = ${includedir}/libyami
libyami_vaapiinclude_HEADERS = $(libyami_vaapi_source_h)
libyami_vaapi_la_SOURCES     = $(libyami_vaapi_source_c)
libyami_vaapi_la_LDFLAGS     = $(libyami_vaapi_ldflags) $(AM_LDFLAGS)
libyami_vaapi_la_CPPFLAGS    = $(libyami_vaapi_cppflags) $(AM_CPPFLAGS)

if ENABLE_TESTS
include Makefile.unittest
endif

DISTCLEANFILES = \
	Makefile.in 


