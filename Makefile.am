
ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

AUTOMAKE_OPTIONS = foreign

# Extra clean files so that maintainer-clean removes *everything*
DISTCLEANFILES = \
	aclocal.m4 autom4te.cache compile config.guess config.sub \
	configure config.h.in config.h.in~ depcomp install-sh ltmain.sh     \
	Makefile.in missing

SUBDIRS = codecparsers common vaapi decoder encoder pkgconfig
if ENABLE_DOCS
	SUBDIRS += doc
endif

if ENABLE_V4L2
SUBDIRS += v4l2
endif

if ENABLE_CAPI
SUBDIRS += capi
endif

if ENABLE_TESTS
SUBDIRS += tests
endif
