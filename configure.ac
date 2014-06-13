#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.68])

## it is interface version for libtool, only change it if you are sure to do so
m4_define([libyami_lt_current], 0)
m4_define([libyami_lt_revision], 2)
m4_define([libyami_lt_age], 0)
m4_define([libyami_lt_version], [libyami_lt_current.libyami_lt_revision.libyami_lt_age])

# package version (lib name suffix), usually sync with git tag
m4_define([libyami_major_version], 0)
m4_define([libyami_minor_version], 0)
m4_define([libyami_micro_version], 2)
m4_define([libyami_version],
                    [libva_major_version.libva_minor_version.libva_micro_version])

AC_INIT([libyami], [libyami_version], [halley.zhao@intel.com])
AC_CONFIG_SRCDIR([common/vaapitypes.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([aux])
AM_INIT_AUTOMAKE([foreign])

LIBYAMI_LT_VERSION="libyami_lt_current:libyami_lt_revision:libyami_lt_age"
LIBYAMI_LT_LDFLAGS="-version-info $LIBYAMI_LT_VERSION"
# "-release" option should be avoid, that create lib name like libyami-1.s0.xxx
AC_SUBST(LIBYAMI_LT_VERSION)
AC_SUBST(LIBYAMI_LT_LDFLAGS)

AC_ARG_ENABLE(debug,
    [AC_HELP_STRING([--enable-debug],[build with extra debug])],
    [case "${enableval}" in
     yes) debug="yes" ;;
     no)  debug="no";;
     *)   AC_MSG_ERROR(bad value for --enable-debug);;
     esac ],
    [debug="no"])

if test "$debug" = "yes"; then
AC_DEFINE([__ENABLE_DEBUG__], [1], [Defined to 1 if --enable-debug="yes" ])
fi

AC_ARG_ENABLE(baytrail,
    [AC_HELP_STRING([--enable-baytrail], [build with baytrail/chromeos support @<:@default=no@:>@])],
    [], [enable_baytrail="no"])

if test "$enable_baytrail" = "yes"; then
AC_DEFINE([__PLATFORM_BYT__], [1], [Defined to 1 if --enable-baytrail="yes" ])
fi

dnl vp8 decoder
AC_ARG_ENABLE(vp8dec,
    [AC_HELP_STRING([--enable-vp8dec], [build with vp8 decoder support @<:@default=no@:>@])],
    [], [enable_vp8dec="yes"])
if test "$enable_vp8dec" = "yes"; then
AC_DEFINE([__BUILD_VP8_DECODER__], [1], [Defined to 1 if --enable-vp8dec="yes" ])
fi
AM_CONDITIONAL(BUILD_VP8_DECODER, test "x$enable_vp8dec" = "xyes")

dnl jpeg decoder
AC_ARG_ENABLE(jpegdec,
    [AC_HELP_STRING([--enable-jpegdec], [build with jpeg decoder support @<:@default=no@:>@])],
    [], [enable_jpegdec="yes"])
if test "$enable_jpegdec" = "yes"; then
AC_DEFINE([__BUILD_JPEG_DECODER__], [1], [Defined to 1 if --enable-jpegdec="yes" ])
fi
AM_CONDITIONAL(BUILD_JPEG_DECODER, test "x$enable_jpegdec" = "xyes")

dnl h264 decoder
AC_ARG_ENABLE(h264dec,
    [AC_HELP_STRING([--enable-h264dec], [build with h264 decoder support @<:@default=no@:>@])],
    [], [enable_h264dec="yes"])
if test "$enable_h264dec" = "yes"; then
AC_DEFINE([__BUILD_H264_DECODER__], [1], [Defined to 1 if --enable-h264dec="yes" ])
fi
AM_CONDITIONAL(BUILD_H264_DECODER, test "x$enable_h264dec" = "xyes")

dnl h264 encoder
AC_ARG_ENABLE(h264enc,
    [AC_HELP_STRING([--enable-h264enc], [build with h264 encoder support @<:@default=no@:>@])],
    [], [enable_h264enc="yes"])
if test "$enable_h264enc" = "yes"; then
AC_DEFINE([__BUILD_H264_ENCODER__], [1], [Defined to 1 if --enable-h264enc="yes" ])
fi
AM_CONDITIONAL(BUILD_H264_ENCODER, test "x$enable_h264enc" = "xyes")

# Checks for programs.
AC_DISABLE_STATIC
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_LIBTOOL
#LT_INIT
# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stddef.h stdint.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

pkgconfigdir=${libdir}/pkgconfig
AC_SUBST(pkgconfigdir)

PKG_CHECK_MODULES(LIBVA, [libva])
PKG_CHECK_MODULES(LIBVA_DRM, [libva-drm])
PKG_CHECK_MODULES(LIBVA_X11, [libva-x11],
	         [AC_DEFINE([HAVE_VA_X11], [1], [Defined to 1 if VA/X11 API is enabled])])

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset strcasecmp strdup])

AC_CONFIG_FILES([Makefile
                 codecparsers/Makefile
                 common/Makefile
                 decoder/Makefile
                 encoder/Makefile
                 pkgconfig/Makefile])

AC_OUTPUT([
         pkgconfig/libcodecparser.pc
         pkgconfig/libyami_common.pc
         pkgconfig/libyami_decoder.pc
         pkgconfig/libyami_encoder.pc
])

