INCLUDES = -I$(top_srcdir)

libyami_common_source_c = \
	log.cpp \
	utils.cpp \
	nalreader.cpp \
	$(NULL)

libyami_common_source_h_priv = \
	log.h \
	utils.h \
	common_def.h \
	nalreader.h \
	$(NULL)

libyami_common_ldflags = \
	$(LIBYAMI_LT_LDFLAGS) \
	$(LIBVA_LIBS) \
	$(LIBVA_DRM_LIBS) \
	$(NULL)

libyami_common_cppflags = \
	$(LIBVA_CFLAGS) \
	$(NULL)

lib_LTLIBRARIES               = libyami_common.la
libyami_commonincludedir      = ${includedir}/libyami_common
libyami_commoninclude_HEADERS = $(libyami_common_source_h)
libyami_common_la_SOURCES     = $(libyami_common_source_c)
libyami_common_la_LDFLAGS     = $(libyami_common_ldflags)
libyami_common_la_CPPFLAGS    = $(libyami_common_cppflags)

DISTCLEANFILES = \
	Makefile.in 


