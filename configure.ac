#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

m4_define([libva_version], [0.34])
AC_PREREQ([2.68])
AC_INIT([libvacodec], [1.0.0], [xiaowei.a.li@intel.com])
AC_CONFIG_SRCDIR([common/vaapitypes.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([aux])
AM_INIT_AUTOMAKE([foreign])

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

PKG_CHECK_MODULES(LIBVA, [libva >= libva_version])
PKG_CHECK_MODULES(LIBVA_X11, [libva-x11],
	         [AC_DEFINE([HAVE_VA_X11], [1], [Defined to 1 if VA/X11 API is enabled])])

#AC_SUBST([HAVE_VA_X11])
#AC_SUBST([LIBVA_LIBS])
#AC_SUBST([LIBVA_CFLAGS])

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset strcasecmp strdup])

AC_CONFIG_FILES([Makefile
                 codecparsers/Makefile
                 common/Makefile
                 decoder/Makefile])

AC_OUTPUT([
         pkgconfig/libcodecparser.pc
         pkgconfig/libvacodec_decoder.pc
])

