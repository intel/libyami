
ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

AUTOMAKE_OPTIONS = foreign

# Extra clean files so that maintainer-clean removes *everything*
DISTCLEANFILES = \
	aclocal.m4 autom4te.cache compile config.guess config.sub \
	configure config.h.in config.h.in~ depcomp install-sh ltmain.sh \
	Makefile.in missing

SUBDIRS = common codecparsers vaapi decoder encoder vpp pkgconfig
if ENABLE_DOCS
SUBDIRS += doc
endif

if ENABLE_CAPI
SUBDIRS += capi
endif

#generate libyami.la for ENABLE_TESTS and ENABLE_V4L2
SUBDIRS += .

if ENABLE_V4L2
SUBDIRS += v4l2
endif

if ENABLE_TESTS
SUBDIRS += tests
SUBDIRS += testscripts
SUBDIRS += examples
endif

lib_LTLIBRARIES = libyami.la
libyami_la_SOURCES =
libyami_la_LDFLAGS = \
	$(LIBYAMI_LT_LDFLAGS) \
	$(LIBVA_LIBS) \
	$(LIBVA_DRM_LIBS) \
	$(NULL)

libyami_la_CPPFLAGS = \
	$(LIBVA_CFLAGS) \
	$(LIBVA_DRM_CFLAGS) \
	$(NULL)

libyami_la_LIBADD = common/libyami_common.la \
	codecparsers/libyami_codecparser.la \
	vaapi/libyami_vaapi.la \
	decoder/libyami_decoder.la \
	encoder/libyami_encoder.la \
	vpp/libyami_vpp.la \
	$(NULL)

if ENABLE_CAPI
libyami_la_LIBADD += capi/libyami_capi.la
endif

if ENABLE_X11
libyami_la_LDFLAGS += $(LIBVA_X11_LIBS) $(X11_LIBS)
libyami_la_CPPFLAGS += $(LIBVA_X11_CFLAGS)
endif
