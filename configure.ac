#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.68])

## it is interface version for libtool, only change it if you are sure to do so
m4_define([libyami_lt_current], 0)
m4_define([libyami_lt_revision], 3)
m4_define([libyami_lt_age], 0)
m4_define([libyami_lt_version], [libyami_lt_current.libyami_lt_revision.libyami_lt_age])

# package version (lib name suffix), usually sync with git tag
m4_define([libyami_major_version], 0)
m4_define([libyami_minor_version], 3)
# even number of micro_version means a release after full validation cycle
m4_define([libyami_micro_version], 0)
m4_define([libyami_version],
                    [libyami_major_version.libyami_minor_version.libyami_micro_version])

# libva minimun version requirement
m4_define([va_api_version], [0.36])

AC_INIT([libyami],
        [libyami_version],
        [https://github.com/01org/libyami/issues/new],
        [libyami],
        [https://github.com/01org/libyami])

AC_CONFIG_SRCDIR([vaapi/vaapitypes.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([aux])
AM_INIT_AUTOMAKE([foreign])

LIBYAMI_LT_VERSION="libyami_major_version:libyami_minor_version:libyami_micro_version"
LIBYAMI_LT_LDFLAGS="-version-number $LIBYAMI_LT_VERSION"
# "-release" option should be avoid, that create lib name like libyami-1.s0.xxx
AC_SUBST(LIBYAMI_LT_VERSION)
AC_SUBST(LIBYAMI_LT_LDFLAGS)

AC_ARG_ENABLE(debug,
    [AC_HELP_STRING([--enable-debug],
        [build with extra debug @<:@default=no@:>@])],
    [], [enable_debug="no"])

if test "$enable_debug" = "yes"; then
    AC_DEFINE([__ENABLE_DEBUG__], [1],
        [Defined to 1 if --enable-debug="yes"])
fi

AC_ARG_ENABLE(tests,
    [AC_HELP_STRING([--enable-tests],
        [build tests @<:@default=no@:>@])],
    [], [enable_tests="no"])

AC_ARG_ENABLE(tests-gles,
    [AC_HELP_STRING([--enable-tests-gles],
        [build tests with opengles/texture rendering @<:@default=no@:>@])],
    [], [enable_tests_gles="no"])

if test "$enable_tests" = "yes" -o "$enable_tests_gles" = "yes"; then
    AC_DEFINE([__ENABLE_TESTS__], [1],
        [Defined to 1 if --enable-tests="yes"])
fi
AM_CONDITIONAL(ENABLE_TESTS,
    [test "x$enable_tests" = "xyes" -o "x$enable_tests_gles" = "xyes"])

if test "$enable_tests_gles" = "yes"; then
    AC_DEFINE([__ENABLE_TESTS_GLES__], [1],
        [Defined to 1 if --enable-tests-gles="yes"])
fi
AM_CONDITIONAL(ENABLE_TESTS_GLES,
    [test "x$enable_tests_gles" = "xyes"])

AC_ARG_ENABLE(dmabuf,
    [AC_HELP_STRING([--enable-dmabuf],
        [support dma_buf buffer sharing @<:@default=no@:>@])],
    [], [enable_dmabuf="no"])

if test "$enable_dmabuf" = "yes"; then
    AC_DEFINE([__ENABLE_DMABUF__], [1],
        [Defined to 1 if --enable-dmabuf="yes"])
fi
AM_CONDITIONAL(ENABLE_DMABUF,
    [test "x$enable_dmabuf" = "xyes"])

AC_ARG_ENABLE(v4l2,
    [AC_HELP_STRING([--enable-v4l2],
        [wrapper of v4l2 interface @<:@default=no@:>@])],
    [], [enable_v4l2="no"])

AC_ARG_ENABLE(v4l2-glx,
    [AC_HELP_STRING([--enable-v4l2-glx],
        [use glx/tfp in v4l2 wrapper @<:@default=no@:>@])],
    [], [enable_v4l2_glx="no"])

if test "$enable_v4l2" = "yes" -o "$enable_v4l2_glx" = "yes"; then
    AC_DEFINE([__ENABLE_V4L2__], [1],
        [Defined to 1 if --enable-v4l2="yes" or --enable-v4l2-glx="yes"])
fi
AM_CONDITIONAL(ENABLE_V4L2,
    [test "x$enable_v4l2" = "xyes" -o "x$enable_v4l2_glx" = "xyes"])

if test "$enable_v4l2_glx" = "yes"; then
    AC_DEFINE([__ENABLE_V4L2_GLX__], [1],
        [Defined to 1 if --enable-v4l2-glx="yes"])
fi
AM_CONDITIONAL(ENABLE_V4L2_GLX,
    [test "x$enable_v4l2_glx" = "xyes"])

AC_ARG_ENABLE(capi,
    [AC_HELP_STRING([--enable-capi],
        [wrapper of c api interface @<:@default=no@:>@])],
    [], [enable_capi="no"])

if test "$enable_capi" = "yes"; then
    AC_DEFINE([__ENABLE_CAPI__], [1],
        [Defined to 1 if --enable-capi="yes"])
fi
AM_CONDITIONAL(ENABLE_CAPI,
    [test "x$enable_capi" = "xyes"])

AC_ARG_ENABLE(x11,
    [AC_HELP_STRING([--enable-x11],
        [enable x11 @<:@default=yes@:>@])],
    [], [enable_x11="yes"])

if test "$enable_x11" = "yes"; then
    AC_DEFINE([__ENABLE_X11__], [1],
        [Defined to 1 if --enable-x11="yes"])
fi
AM_CONDITIONAL(ENABLE_X11,
    [test "x$enable_x11" = "xyes"])

AC_ARG_ENABLE(avformat,
    [AC_HELP_STRING([--enable-avformat],
        [enable avformat @<:@default=no@:>@])],
    [], [enable_avformat="no"])

if test "$enable_avformat" = "yes"; then
    AC_DEFINE([__ENABLE_AVFORMAT__], [1],
        [Defined to 1 if --enable-avformat="yes"])
fi
AM_CONDITIONAL(ENABLE_AVFORMAT,
    [test "x$enable_avformat" = "xyes"])

AC_ARG_ENABLE(baytrail,
    [AC_HELP_STRING([--enable-baytrail],
        [build with baytrail/chromeos support @<:@default=no@:>@])],
    [], [enable_baytrail="no"])

if test "$enable_baytrail" = "yes"; then
    AC_DEFINE([__PLATFORM_BYT__], [1],
        [Defined to 1 if --enable-baytrail="yes"])
fi

AC_ARG_ENABLE(md5,
    [AC_HELP_STRING([--enable-md5], [enable generate md5 by per frame@<:@default=yes@:>@])],
    [], [enable_md5="yes"])

dnl vp8 decoder
AC_ARG_ENABLE(vp8dec,
    [AC_HELP_STRING([--enable-vp8dec],
        [build with vp8 decoder support @<:@default=yes@:>@])],
    [], [enable_vp8dec="yes"])

AM_CONDITIONAL(BUILD_VP8_DECODER,
    [test "x$enable_vp8dec" = "xyes"])

dnl vp9 decoder
AC_ARG_ENABLE(vp9dec,
    [AC_HELP_STRING([--enable-vp9dec],
        [build with vp9 decoder support @<:@default=no@:>@])],
    [], [enable_vp9dec="no"])

AM_CONDITIONAL(BUILD_VP9_DECODER,
    [test "x$enable_vp9dec" = "xyes"])

dnl jpeg decoder
AC_ARG_ENABLE(jpegdec,
    [AC_HELP_STRING([--enable-jpegdec],
        [build with jpeg decoder support @<:@default=yes@:>@])],
    [], [enable_jpegdec="yes"])

AM_CONDITIONAL(BUILD_JPEG_DECODER,
    [test "x$enable_jpegdec" = "xyes"])

dnl h264 decoder
AC_ARG_ENABLE(h264dec,
    [AC_HELP_STRING([--enable-h264dec],
        [build with h264 decoder support @<:@default=yes@:>@])],
    [], [enable_h264dec="yes"])

AM_CONDITIONAL(BUILD_H264_DECODER,
    [test "x$enable_h264dec" = "xyes"])

dnl h265 decoder
AC_ARG_ENABLE(h265dec,
    [AC_HELP_STRING([--enable-h265dec],
        [build with h265 decoder support @<:@default=yes@:>@])],
    [], [enable_h265dec="yes"])

AM_CONDITIONAL(BUILD_H265_DECODER,
    [test "x$enable_h265dec" = "xyes"])

dnl fake decoder
AC_ARG_ENABLE(fakedec,
    [AC_HELP_STRING([--enable-fakedec],
        [build with fake decoder support @<:@default=no@:>@])],
    [], [enable_fakedec="no"])

if test "$enable_fakedec" = "yes"; then
    AC_DEFINE([__BUILD_FAKE_DECODER__], [1],
        [Defined to 1 if --enable-fakedec="yes"])
fi
AM_CONDITIONAL(BUILD_FAKE_DECODER,
    [test "x$enable_fakedec" = "xyes"])

dnl h264 encoder
AC_ARG_ENABLE(h264enc,
    [AC_HELP_STRING([--enable-h264enc],
        [build with h264 encoder support @<:@default=yes@:>@])],
    [], [enable_h264enc="yes"])

AM_CONDITIONAL(BUILD_H264_ENCODER,
    [test "x$enable_h264enc" = "xyes"])

dnl jpeg encoder
AC_ARG_ENABLE(jpegenc,
    [AC_HELP_STRING([--enable-jpegenc],
        [build with jpeg encoder support @<:@default=no@:>@])],
    [], [enable_jpegenc="no"])

AM_CONDITIONAL(BUILD_JPEG_ENCODER,
    [test "x$enable_jpegenc" = "xyes"])

dnl vp8 encoder
AC_ARG_ENABLE(vp8enc,
    [AC_HELP_STRING([--enable-vp8enc],
        [build with vp8 encoder support @<:@default=no@:>@])],
    [], [enable_vp8enc="no"])

AM_CONDITIONAL(BUILD_VP8_ENCODER,
    [test "x$enable_vp8enc" = "xyes"])

dnl h265 encoder
AC_ARG_ENABLE(h265enc,
    [AC_HELP_STRING([--enable-h265enc],
        [build with h265 encoder support @<:@default=no@:>@])],
    [], [enable_h265enc="no"])

AM_CONDITIONAL(BUILD_H265_ENCODER,
    [test "x$enable_h265enc" = "xyes"])

dnl encoder getmv
AC_ARG_ENABLE(getmv,
    [AC_HELP_STRING([--enable-getmv],
        [build with get mv support @<:@default=no@:>@])],
    [], [enable_getmv="no"])

if test "$enable_getmv" = "yes"; then
    AC_DEFINE([__BUILD_GET_MV__], [1],
        [Defined to 1 if --enable-getmv="yes"])
fi
AM_CONDITIONAL(BUILD_GET_MV,
    [test "x$enable_getmv" = "xyes"])


# dnl Doxygen
AC_ARG_ENABLE(docs,
    [AC_HELP_STRING([--enable-docs],
        [build Doxygen docs @<:@default=yes@:>@])],
    [], [enable_docs="yes"])

if test "$enable_docs" = "yes"; then
    AC_CHECK_TOOL([DOXYGEN], [doxygen], [no])
    if test "$DOXYGEN" = "no"; then
       enable_docs="no"
    fi
fi
AM_CONDITIONAL(ENABLE_DOCS,
    [test "$enable_docs" = "yes"])

: ${CFLAGS="-g -O2 -Wno-unused-function -Wno-sign-compare -Wno-cpp -Wall -Werror"}
: ${CXXFLAGS="-g -O2 -Wno-unused-function -Wno-sign-compare -Wno-cpp -Wall -Werror"}

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

# Check for libva (vaapi version)
VA_API_VERSION=va_api_version
AX_PKG_CHECK_MODULES([LIBVA], [libva >= $VA_API_VERSION], [], [], [],
    [AX_LIBVA_PKG_REQ], [AX_LIBVA_PKG_REQ_PRIV])
AC_SUBST(LIBVA_VERSION)

AX_PKG_CHECK_MODULES([LIBVA_DRM], [libva-drm], [], [], [],
    [AX_LIBVA_DRM_PKG_REQ], [AX_LIBVA_DRM_PKG_REQ_PRIV])
if test "$enable_x11" = "yes" -o "$enable_v4l2_glx" = "yes"; then
    AX_PKG_CHECK_MODULES([LIBVA_X11], [libva-x11], [],
        [AC_DEFINE([HAVE_VA_X11], [1],
            [Defined to 1 if VA/X11 API is enabled])],
        [], [AX_LIBVA_X11_PKG_REQ], [AX_LIBVA_X11_PKG_REQ_PRIV])
    AX_PKG_CHECK_MODULES([X11], [x11], [], [], [],
        [AX_X11_PKG_REQ], [AX_X11_PKG_REQ_PRIV])
fi

if test "$enable_v4l2" = "yes"; then
    AX_PKG_CHECK_MODULES([LIBV4L2], [libv4l2], [], [], [],
        [AX_LIBV4L2_PKG_REQ], [AX_LIBV4L2_PKG_REQ_PRIV])
fi

if test ["$enable_v4l2" = "yes" -a "$enable_v4l2_glx" = "no"] -o "$enable_tests_gles" = "yes"; then
    AX_PKG_CHECK_MODULES([LIBEGL], [egl], [], [], [],
        [AX_LIBEGL_PKG_REQ], [AX_LIBEGL_PKG_REQ_PRIV])
fi

if test ["$enable_v4l2" = "yes" -a "$enable_v4l2_glx" = "no"  -a "$enable_tests" = "yes"] -o "$enable_tests_gles" = "yes"; then
    AX_PKG_CHECK_MODULES([LIBGLES2], [glesv2], [], [], [],
        [AX_LIBGLES2_PKG_REQ], [AX_LIBGLES2_PKG_REQ_PRIV])
fi

# drm_fourcc.h for dma_buf support
if test "$enable_dmabuf" = "yes"; then
    AX_PKG_CHECK_MODULES([LIBDRM], [libdrm], [], [], [],
        [AX_LIBDRM_PKG_REQ], [AX_LIBDRM_PKG_REQ_PRIV])
fi

if test "$enable_v4l2_glx" = "yes"; then
    AX_PKG_CHECK_MODULES([LIBGL], [gl], [], [], [],
        [AX_LIBGL_PKG_REQ], [AX_LIBGL_PKG_REQ_PRIV])
fi

if test "$enable_avformat" = "yes"; then
    AX_PKG_CHECK_MODULES(LIBAVFORMAT,  [libavformat libavcodec libavutil],
        [], [], [], [AX_LIBAVFORMAT_PKG_REQ], [AX_LIBAVFORMAT_PKG_REQ_PRIV])
fi

#check openssl
if test "$enable_md5" = "yes"; then
    AX_PKG_CHECK_MODULES([OPENSSL], [openssl], [],
        [AC_DEFINE([__ENABLE_MD5__], [1],
            [Defined to 1 if MD5 API and --enable-md5[default] are enabled])],
        [enable_md5="no"], [AX_OPENSSL_PKG_REQ], [AX_OPENSSL_PKG_REQ_PRIV])
fi
AM_CONDITIONAL(ENABLE_MD5, test "x$enable_md5" = "xyes")

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
                 vpp/Makefile
                 v4l2/Makefile
                 capi/Makefile
                 tests/Makefile
                 testscripts/Makefile
                 examples/Makefile
                 doc/Makefile
                 pkgconfig/Makefile])

AC_OUTPUT([
         pkgconfig/libyami_codecparser.pc
         pkgconfig/libyami_common.pc
         pkgconfig/libyami_decoder.pc
         pkgconfig/libyami_encoder.pc
         pkgconfig/libyami_vaapi.pc
         pkgconfig/libyami_vpp.pc
])

# Print a configuration summary
DECODERS=""
AS_IF([test x$enable_h265dec = xyes], [DECODERS="$DECODERS h265"])
AS_IF([test x$enable_h264dec = xyes], [DECODERS="$DECODERS h264"])
AS_IF([test x$enable_jpegdec = xyes], [DECODERS="$DECODERS jpeg"])
AS_IF([test x$enable_vp8dec = xyes], [DECODERS="$DECODERS vp8"])
AS_IF([test x$enable_vp9dec = xyes], [DECODERS="$DECODERS vp9"])
AS_IF([test x$enable_fakedec = xyes], [DECODERS="$DECODERS fake"])

ENCODERS=""
AS_IF([test x$enable_h265enc = xyes], [ENCODERS="$ENCODERS h265"])
AS_IF([test x$enable_h264enc = xyes], [ENCODERS="$ENCODERS h264"])
AS_IF([test x$enable_jpegenc = xyes], [ENCODERS="$ENCODERS jpeg"])
AS_IF([test x$enable_vp8enc = xyes], [ENCODERS="$ENCODERS vp8"])

AC_MSG_RESULT([
    libyami - libyami_version

    Build decoders ................... :$DECODERS
    Build encoders ....................:$ENCODERS
    Build documentation .............. : $enable_docs
    Enable debug ......................: $enable_debug
    Installation prefix .............. : $prefix
])
