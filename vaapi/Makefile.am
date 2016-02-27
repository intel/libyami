INCLUDES = -I$(top_srcdir)

libyami_vaapi_source_c = \
	vaapipicture.cpp \
	vaapibuffer.cpp \
	vaapiimage.cpp \
	vaapisurface.cpp\
	vaapiimageutils.cpp \
	vaapiutils.cpp \
	vaapidisplay.cpp \
	vaapicontext.cpp \
	vaapisurfaceallocator.cpp \
	$(NULL)

libyami_vaapi_source_h_priv = \
	vaapipicture.h \
	vaapibuffer.h \
	vaapiimage.h \
	vaapisurface.h \
	vaapiimageutils.h \
	vaapiutils.h \
	vaapitypes.h \
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

DISTCLEANFILES = \
	Makefile.in 


