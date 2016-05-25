pkg_config_files = \
	libyami.pc \
	$(NULL)

if ENABLE_V4L2
pkg_config_files += \
	libyami_v4l2.pc \
	$(NULL)
endif

pkgconfigdir = @pkgconfigdir@
pkgconfig_DATA = $(pkg_config_files)

DISTCLEANFILES = \
	$(pkg_config_files) \
	Makefile.in \
	$(NULL)
