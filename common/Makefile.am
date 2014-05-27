libyami_common_source_c = \
        vaapipicture.cpp \
        vaapibuffer.cpp \
        vaapiimage.cpp \
        vaapisurface.cpp\
        vaapiutils.cpp \
	$(NULL)

libyami_common_source_h = \
        vaapipicture.h \
        vaapibuffer.h \
        vaapiimage.h \
        vaapisurface.h \
        vaapiutils.h \
        vaapitypes.h \
        log.h \
	$(NULL)

libyami_common_ldflags = \
        $(LIBYAMI_LT_LDFLAGS) \
        $(LIBVA_LIBS) \
        $(LIBVA_DRM_LIBS) \
	$(NULL)

libyami_common_cppflags = \
        $(LIBVA_CFLAGS) \
        -fpermissive \
	$(NULL)

lib_LTLIBRARIES	              = libyami_common.la
libyami_commonincludedir      = ${includedir}/libyami_common
libyami_commoninclude_HEADERS = $(libyami_common_source_h)
libyami_common_la_SOURCES     = $(libyami_common_source_c)
libyami_common_la_LDFLAGS     = $(libyami_common_ldflags)
libyami_common_la_CPPFLAGS    = $(libyami_common_cppflags)

DISTCLEANFILES = \
	Makefile.in 


