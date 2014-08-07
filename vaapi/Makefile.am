libyami_vaapi_source_c = \
        vaapipicture.cpp \
        vaapibuffer.cpp \
        vaapiimage.cpp \
        vaapisurface.cpp\
        vaapiutils.cpp \
        vaapidisplay.cpp \
        vaapicontext.cpp \
	$(NULL)

libyami_vaapi_source_h = \
        vaapipicture.h \
        vaapibuffer.h \
        vaapiimage.h \
        vaapisurface.h \
        vaapiutils.h \
        vaapitypes.h \
        vaapidisplay.h \
        vaapicontext.h \
	$(NULL)

libyami_vaapi_ldflags = \
        $(LIBYAMI_LT_LDFLAGS) \
        $(LIBVA_LIBS) \
        $(LIBVA_DRM_LIBS) \
        $(LIBVA_X11_LIBS) \
	$(NULL)

libyami_vaapi_cppflags = \
        $(LIBVA_CFLAGS) \
        -fpermissive \
	$(NULL)

lib_LTLIBRARIES	              = libyami_vaapi.la
libyami_vaapiincludedir      = ${includedir}/libyami_vaapi
libyami_vaapiinclude_HEADERS = $(libyami_vaapi_source_h)
libyami_vaapi_la_SOURCES     = $(libyami_vaapi_source_c)
libyami_vaapi_la_LDFLAGS     = $(libyami_vaapi_ldflags)
libyami_vaapi_la_CPPFLAGS    = $(libyami_vaapi_cppflags)

DISTCLEANFILES = \
	Makefile.in 


