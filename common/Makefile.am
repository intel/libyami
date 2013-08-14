libvacodec_common_source_c = \
        vaapibuffer.cpp \
        vaapiimage.cpp \
        vaapisurface.cpp\
        vaapiutils.cpp \
	$(NULL)

libvacodec_common_source_h = \
        vaapibuffer.h \
        vaapiimage.h \
        vaapisurface.h \
        vaapiutils.h \
        vaapitypes.h \
        log.h \
	$(NULL)

libvacodec_common_ldflags = \
        $(LIBVA_LIBS) \
	$(NULL)

libvacodec_common_cppflags = \
        $(LIBVA_CFLAGS) \
        -fpermissive \
	$(NULL)

lib_LTLIBRARIES			 = libvacodec_common.la
libvacodec_commonincludedir      = ${includedir}/libvacodec/libvacodec_common
libvacodec_commoninclude_HEADERS = $(libvacodec_common_source_h)

libvacodec_common_la_SOURCES	 = $(libvacodec_common_source_c)
libvacodec_common_la_LDFLAGS	 = $(libvacodec_common_ldflags)
libvacodec_common_la_CPPFLAGS    = $(libvacodec_common_cppflags)

DISTCLEANFILES = \
	Makefile.in 


