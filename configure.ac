#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.68])
AC_INIT([libyami], [0.2.0], [xiaowei.a.li@intel.com])
AC_CONFIG_SRCDIR([vaapi/vaapitypes.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([aux])
AM_INIT_AUTOMAKE([foreign])

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
                 pkgconfig/Makefile])

AC_OUTPUT([
         pkgconfig/libcodecparser.pc
         pkgconfig/libyami_common.pc
         pkgconfig/libyami_decoder.pc
])

