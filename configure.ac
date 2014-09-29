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
AC_CONFIG_SRCDIR([vaapi/vaapitypes.h])
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

AC_ARG_ENABLE(tests,
    [AC_HELP_STRING([--enable-tests], [build tests @<:@default=no@:>@])],
    [], [enable_tests="no"])

if test "$enable_tests" = "yes"; then
AC_DEFINE([__ENABLE_TESTS__], [1], [Defined to 1 if --enable-tests="yes" ])
fi
AM_CONDITIONAL(ENABLE_TESTS, test "x$enable_tests" = "xyes")


AC_ARG_ENABLE(tests-gles,
    [AC_HELP_STRING([--enable-tests-gles], [build tests with opengles/texutre rendering@<:@default=no@:>@])],
    [], [enable_tests_gles="no"])

if test "$enable_tests_gles" = "yes"; then
AC_DEFINE([__ENABLE_TESTS_GLES__], [1], [Defined to 1 if --enable-tests-gles="yes" ])
fi
AM_CONDITIONAL(ENABLE_TESTS_GLES, test "x$enable_tests_gles" = "xyes")

AC_ARG_ENABLE(dmabuf,
    [AC_HELP_STRING([--enable-dmabuf], [support dm_buf buffer sharing@<:@default=no@:>@])],
    [], [enable_dmabuf="no"])

if test "$enable_dmabuf" = "yes"; then
AC_DEFINE([__ENABLE_DMABUF__], [1], [Defined to 1 if --enable-dmabuf="yes" ])
fi
AM_CONDITIONAL(ENABLE_DMABUF, test "x$enable_dmabuf" = "xyes")

AC_ARG_ENABLE(v4l2,
    [AC_HELP_STRING([--enable-v4l2], [wrapper of v4l2 interface@<:@default=no@:>@])],
    [], [enable_v4l2="no"])

if test "$enable_v4l2" = "yes"; then
AC_DEFINE([__ENABLE_V4L2__], [1], [Defined to 1 if --enable-v4l2="yes" ])
fi
AM_CONDITIONAL(ENABLE_V4L2, test "x$enable_v4l2" = "xyes")

AC_ARG_ENABLE(capi,
    [AC_HELP_STRING([--enable-capi], [wrapper of c api interface@<:@default=no@:>@])],
    [], [enable_capi="no"])

if test "$enable_capi" = "yes"; then
AC_DEFINE([__ENABLE_CAPI__], [1], [Defined to 1 if --enable-capi="yes" ])
fi
AM_CONDITIONAL(ENABLE_CAPI, test "x$enable_capi" = "xyes")

AC_ARG_ENABLE(x11,
    [AC_HELP_STRING([--enable-x11], [enable x11@<:@default=yes@:>@])],
	[], [enable_x11="yes"])

AS_IF([test "x$enable_x11" = "xyes"], 
[AC_DEFINE([__ENABLE_X11__], [1], [Defined to 0 if --enable-x11="yes" ])],
[TEST "X$enable_x11" = "xno"], 
[AC_DEFINE([__ENABLE_X11__], [1], [Defined to 1 if --disable-x11="yes" ])],
[])
AM_CONDITIONAL(ENABLE_X11, test "x$enable_x11" = "xyes")

AC_ARG_ENABLE(baytrail,
    [AC_HELP_STRING([--enable-baytrail], [build with baytrail/chromeos support @<:@default=no@:>@])],
    [], [enable_baytrail="no"])

if test "$enable_baytrail" = "yes"; then
AC_DEFINE([__PLATFORM_BYT__], [1], [Defined to 1 if --enable-baytrail="yes" ])
fi

dnl vp8 decoder
AC_ARG_ENABLE(vp8dec,
    [AC_HELP_STRING([--enable-vp8dec], [build with vp8 decoder support @<:@default=yes@:>@])],
    [], [enable_vp8dec="yes"])
if test "$enable_vp8dec" = "yes"; then
AC_DEFINE([__BUILD_VP8_DECODER__], [1], [Defined to 1 if --enable-vp8dec="yes" ])
fi
AM_CONDITIONAL(BUILD_VP8_DECODER, test "x$enable_vp8dec" = "xyes")

dnl jpeg decoder
AC_ARG_ENABLE(jpegdec,
    [AC_HELP_STRING([--enable-jpegdec], [build with jpeg decoder support @<:@default=yes@:>@])],
    [], [enable_jpegdec="yes"])
if test "$enable_jpegdec" = "yes"; then
AC_DEFINE([__BUILD_JPEG_DECODER__], [1], [Defined to 1 if --enable-jpegdec="yes" ])
fi
AM_CONDITIONAL(BUILD_JPEG_DECODER, test "x$enable_jpegdec" = "xyes")

dnl h264 decoder
AC_ARG_ENABLE(h264dec,
    [AC_HELP_STRING([--enable-h264dec], [build with h264 decoder support @<:@default=yes@:>@])],
    [], [enable_h264dec="yes"])
if test "$enable_h264dec" = "yes"; then
AC_DEFINE([__BUILD_H264_DECODER__], [1], [Defined to 1 if --enable-h264dec="yes" ])
fi
AM_CONDITIONAL(BUILD_H264_DECODER, test "x$enable_h264dec" = "xyes")

dnl h264 encoder
AC_ARG_ENABLE(h264enc,
    [AC_HELP_STRING([--enable-h264enc], [build with h264 encoder support @<:@default=yes@:>@])],
    [], [enable_h264enc="yes"])
if test "$enable_h264enc" = "yes"; then
AC_DEFINE([__BUILD_H264_ENCODER__], [1], [Defined to 1 if --enable-h264enc="yes" ])
fi
AM_CONDITIONAL(BUILD_H264_ENCODER, test "x$enable_h264enc" = "xyes")

# dnl Doxygen
AC_ARG_ENABLE(docs,
              [AC_HELP_STRING([--enable-docs], [build with doxygen support @<:@default=yes@:>@])],
    [], [enable_docs="yes"])
if test "$enable_docs" = "yes"; then
    AC_CHECK_TOOL([DOXYGEN], [doxygen], [no])
    if test "$DOXYGEN" = "no"; then
       enable_docs="no"
    fi
fi
AM_CONDITIONAL(ENABLE_DOCS, test "$enable_docs" = "yes")

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
if test "$enable_v4l2" = "yes"; then
PKG_CHECK_MODULES(LIBV4L2, [libv4l2])
fi

if test "$enable_v4l2" = "yes" -o "$enable_tests_gles" = "yes"; then
    PKG_CHECK_MODULES(LIBEGL, [egl])
fi

if test ["$enable_v4l2" = "yes" -a "$enable_tests" = "yes"] -o "$enable_tests_gles" = "yes"; then
    PKG_CHECK_MODULES(LIBGLES2, [glesv2])
fi

# drm_fourcc.h for dma_buf support
if test "$enable_dmabuf" = "yes"; then
PKG_CHECK_MODULES(LIBDRM, [libdrm])
fi

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset strcasecmp strdup])

AC_CONFIG_FILES([Makefile
                 codecparsers/Makefile
                 common/Makefile
                 vaapi/Makefile
                 decoder/Makefile
                 encoder/Makefile
                 v4l2/Makefile
                 capi/Makefile
                 tests/Makefile
                 doc/Makefile
                 pkgconfig/Makefile])

AC_OUTPUT([
         pkgconfig/libcodecparser.pc
         pkgconfig/libyami_common.pc
         pkgconfig/libyami_decoder.pc
         pkgconfig/libyami_encoder.pc
])

