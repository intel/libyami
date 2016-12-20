#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.68])

# YAMI-API version
# YAMI-API version for api headers, only change it when the api interface is changed.
m4_define([yami_api_major_version], 0)
# update this for every release when micro version large than zero
m4_define([yami_api_minor_version], 4)
# change this for any api change
m4_define([yami_api_micro_version], 0)
m4_define([yami_api_version],
    [yami_api_major_version.yami_api_minor_version.yami_api_micro_version])

# package version (lib name suffix), usually sync with git tag
m4_define([libyami_major_version], 1)
m4_define([libyami_minor_version], 1)
# even number of micro_version means a release after full validation cycle
m4_define([libyami_micro_version], 0)
m4_define([libyami_version],
                    [libyami_major_version.libyami_minor_version.libyami_micro_version])

# libva minimun version requirement
m4_define([va_min_api_version], [0.36])

AC_INIT([libyami],
        [libyami_version],
        [https://github.com/01org/libyami/issues/new],
        [libyami],
        [https://github.com/01org/libyami])

AC_CONFIG_SRCDIR([common/common_def.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_SILENT_RULES([yes])

LIBYAMI_LT_VERSION="libyami_major_version:libyami_minor_version:libyami_micro_version"
LIBYAMI_LT_LDFLAGS="-version-number $LIBYAMI_LT_VERSION"
# "-release" option should be avoid, that create lib name like libyami-1.s0.xxx
AC_SUBST(LIBYAMI_LT_VERSION)
AC_SUBST(LIBYAMI_LT_LDFLAGS)

YAMI_API_MAJOR_VERSION=yami_api_major_version
YAMI_API_MINOR_VERSION=yami_api_minor_version
YAMI_API_MICRO_VERSION=yami_api_micro_version
YAMI_API_VERSION=yami_api_version
AC_SUBST(YAMI_API_MAJOR_VERSION)
AC_SUBST(YAMI_API_MINOR_VERSION)
AC_SUBST(YAMI_API_MICRO_VERSION)
AC_SUBST(YAMI_API_VERSION)

AM_CONDITIONAL(BUILD_STATIC,
    [test "x$enable_static" = "xyes"])

AC_ARG_ENABLE(debug,
    [AC_HELP_STRING([--enable-debug],
        [build with extra debug @<:@default=no@:>@])],
    [], [enable_debug="no"])

if test "$enable_debug" = "yes"; then
    AC_DEFINE([__ENABLE_DEBUG__], [1],
        [Defined to 1 if --enable-debug="yes"])
fi

GTEST_LIB_CHECK([1.7.0], [:], [:])
AM_CONDITIONAL([ENABLE_UNITTESTS],
    [test "x$HAVE_GTEST" = "xyes"])

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

if test "$enable_v4l2" = "yes"; then
    AC_DEFINE([__ENABLE_V4L2__], [1],
        [Defined to 1 if --enable-v4l2="yes"])
fi
AM_CONDITIONAL(ENABLE_V4L2,
    [test "x$enable_v4l2" = "xyes"])

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

AC_ARG_ENABLE(egl,
    [AC_HELP_STRING([--enable-egl],
        [enable egl @<:@default=yes@:>@])],
    [], [enable_egl="yes"])

if test "$enable_egl" = "yes"; then
    AC_DEFINE([__ENABLE_EGL__], [1],
        [Defined to 1 if --enable-egl="yes"])
fi
AM_CONDITIONAL(ENABLE_EGL,
    [test "x$enable_egl" = "xyes"])

AC_ARG_ENABLE(wayland,
    [AC_HELP_STRING([--enable-wayland],
        [enable wayland @<:@default=no@:>@])],
    [], [enable_wayland="no"])

if test "$enable_wayland" = "yes"; then
    AC_DEFINE([__ENABLE_WAYLAND__], [1],
        [Defined to 1 if --enable-wayland="yes"])
fi
AM_CONDITIONAL(ENABLE_WAYLAND,
    [test "x$enable_wayland" = "xyes"])

AC_ARG_ENABLE(baytrail,
    [AC_HELP_STRING([--enable-baytrail],
        [build with baytrail/chromeos support @<:@default=no@:>@])],
    [], [enable_baytrail="no"])

if test "$enable_baytrail" = "yes"; then
    AC_DEFINE([__PLATFORM_BYT__], [1],
        [Defined to 1 if --enable-baytrail="yes"])
fi

AC_ARG_ENABLE(v4l2-ops,
    [AC_HELP_STRING([--enable-v4l2-ops],
        [support v4l2codec_device_ops.h interface in v4l2 wrapper @<:@default=no@:>@])],
    [], [enable_v4l2_ops="no"])

if test "$enable_v4l2_ops" = "yes"; then
    AC_DEFINE([__ENABLE_V4L2_OPS__], [1],
        [Defined to 1 if --enable-v4l2-ops="yes"])
fi
AM_CONDITIONAL(ENABLE_V4L2_OPS,
    [test "x$enable_v4l2_ops" = "xyes"])

AC_ARG_ENABLE(md5,
    [AC_HELP_STRING([--enable-md5], [enable generate md5 by per frame@<:@default=yes@:>@])],
    [], [enable_md5="yes"])

dnl mpeg2 decoder
AC_ARG_ENABLE(mpeg2dec,
    [AC_HELP_STRING([--enable-mpeg2dec],
        [build with mpeg2 decoder support @<:@default=no@:>@])],
    [], [enable_mpeg2dec="no"])

if test "$enable_mpeg2dec" = "yes"; then
    AC_DEFINE([__BUILD_MPEG2_DECODER__], [1],
        [Defined to 1 if --enable-mpeg2dec="yes"])
fi

AM_CONDITIONAL(BUILD_MPEG2_DECODER,
    [test "x$enable_mpeg2dec" = "xyes"])

dnl vp8 decoder
AC_ARG_ENABLE(vp8dec,
    [AC_HELP_STRING([--enable-vp8dec],
        [build with vp8 decoder support @<:@default=yes@:>@])],
    [], [enable_vp8dec="yes"])

if test "$enable_vp8dec" = "yes"; then
    AC_DEFINE([__BUILD_VP8_DECODER__], [1],
        [Defined to 1 if --enable-vp8dec="yes"])
fi

AM_CONDITIONAL(BUILD_VP8_DECODER,
    [test "x$enable_vp8dec" = "xyes"])

dnl vp9 decoder
AC_ARG_ENABLE(vp9dec,
    [AC_HELP_STRING([--enable-vp9dec],
        [build with vp9 decoder support @<:@default=no@:>@])],
    [], [enable_vp9dec="no"])

if test "$enable_vp9dec" = "yes"; then
    AC_DEFINE([__BUILD_VP9_DECODER__], [1],
        [Defined to 1 if --enable-vp9dec="yes"])
fi

AM_CONDITIONAL(BUILD_VP9_DECODER,
    [test "x$enable_vp9dec" = "xyes"])

dnl jpeg decoder
AC_ARG_ENABLE(jpegdec,
    [AC_HELP_STRING([--enable-jpegdec],
        [build with jpeg decoder support @<:@default=yes@:>@])],
    [], [enable_jpegdec="yes"])

if test "$enable_jpegdec" = "yes"; then
    AC_DEFINE([__BUILD_JPEG_DECODER__], [1],
        [Defined to 1 if --enable-jpegdec="yes"])
fi

AM_CONDITIONAL(BUILD_JPEG_DECODER,
    [test "x$enable_jpegdec" = "xyes"])

dnl h264 decoder
AC_ARG_ENABLE(h264dec,
    [AC_HELP_STRING([--enable-h264dec],
        [build with h264 decoder support @<:@default=yes@:>@])],
    [], [enable_h264dec="yes"])

if test "$enable_h264dec" = "yes"; then
    AC_DEFINE([__BUILD_H264_DECODER__], [1],
        [Defined to 1 if --enable-h264dec="yes"])
fi

AM_CONDITIONAL(BUILD_H264_DECODER,
    [test "x$enable_h264dec" = "xyes"])

dnl h265 decoder
AC_ARG_ENABLE(h265dec,
    [AC_HELP_STRING([--enable-h265dec],
        [build with h265 decoder support @<:@default=yes@:>@])],
    [], [enable_h265dec="yes"])

if test "$enable_h265dec" = "yes"; then
    AC_DEFINE([__BUILD_H265_DECODER__], [1],
        [Defined to 1 if --enable-h265dec="yes"])
fi

AM_CONDITIONAL(BUILD_H265_DECODER,
    [test "x$enable_h265dec" = "xyes"])

dnl vc1 decoder
AC_ARG_ENABLE(vc1dec,
    [AC_HELP_STRING([--enable-vc1dec],
        [build with vc1 decoder support @<:@default=no@:>@])],
    [], [enable_vc1dec="no"])

if test "$enable_vc1dec" = "yes"; then
    AC_DEFINE([__BUILD_VC1_DECODER__], [1],
        [Defined to 1 if --enable-vc1dec="yes"])
fi

AM_CONDITIONAL(BUILD_VC1_DECODER,
    [test "x$enable_vc1dec" = "xyes"])

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

if test "$enable_h264enc" = "yes"; then
    AC_DEFINE([__BUILD_H264_ENCODER__], [1],
        [Defined to 1 if --enable-h264enc="yes"])
fi

AM_CONDITIONAL(BUILD_H264_ENCODER,
    [test "x$enable_h264enc" = "xyes"])

dnl jpeg encoder
AC_ARG_ENABLE(jpegenc,
    [AC_HELP_STRING([--enable-jpegenc],
        [build with jpeg encoder support @<:@default=no@:>@])],
    [], [enable_jpegenc="no"])

if test "$enable_jpegenc" = "yes"; then
    AC_DEFINE([__BUILD_JPEG_ENCODER__], [1],
        [Defined to 1 if --enable-jpegenc="yes"])
fi

AM_CONDITIONAL(BUILD_JPEG_ENCODER,
    [test "x$enable_jpegenc" = "xyes"])

dnl vp8 encoder
AC_ARG_ENABLE(vp8enc,
    [AC_HELP_STRING([--enable-vp8enc],
        [build with vp8 encoder support @<:@default=no@:>@])],
    [], [enable_vp8enc="no"])

if test "$enable_vp8enc" = "yes"; then
    AC_DEFINE([__BUILD_VP8_ENCODER__], [1],
        [Defined to 1 if --enable-vp8enc="yes"])
fi

AM_CONDITIONAL(BUILD_VP8_ENCODER,
    [test "x$enable_vp8enc" = "xyes"])

dnl vp9 encoder
AC_ARG_ENABLE(vp9enc,
    [AC_HELP_STRING([--enable-vp9enc],
        [build with vp9 encoder support @<:@default=no@:>@])],
    [], [enable_vp9enc="no"])

if test "$enable_vp9enc" = "yes"; then
    AC_DEFINE([__BUILD_VP9_ENCODER__], [1],
        [Defined to 1 if --enable-vp9enc="yes"])
fi

AM_CONDITIONAL(BUILD_VP9_ENCODER,
    [test "x$enable_vp9enc" = "xyes"])

dnl h265 encoder
AC_ARG_ENABLE(h265enc,
    [AC_HELP_STRING([--enable-h265enc],
        [build with h265 encoder support @<:@default=no@:>@])],
    [], [enable_h265enc="no"])

if test "$enable_h265enc" = "yes"; then
    AC_DEFINE([__BUILD_H265_ENCODER__], [1],
        [Defined to 1 if --enable-h265enc="yes"])
fi

AM_CONDITIONAL(BUILD_H265_ENCODER,
    [test "x$enable_h265enc" = "xyes"])

dnl jpeg parser
AM_CONDITIONAL(BUILD_JPEG_PARSER,
    [test "x$enable_jpegdec" = "xyes" -o "x$enable_jpegenc" = "xyes"])

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

dnl ocl filters
AC_ARG_ENABLE(oclfilters,
    [AC_HELP_STRING([--enable-oclfilters],
        [build with opencl based alpha blend support @<:@default=no@:>@])],
    [], [enable_oclfilters="no"])

if test "$enable_oclfilters" = "yes"; then
    AC_DEFINE([__BUILD_OCL_FILTERS__], [1],
        [Defined to 1 if --enable-oclfilters="yes"])
fi

AM_CONDITIONAL(BUILD_OCL_FILTERS,
    [test "x$enable_oclfilters" = "xyes"])


# dnl Doxygen
AC_ARG_ENABLE(docs,
    [AC_HELP_STRING([--enable-docs],
        [build Doxygen docs @<:@default=no@:>@])],
    [], [enable_docs="no"])

if test "$enable_docs" = "yes"; then
    AC_CHECK_TOOL([DOXYGEN], [doxygen], [no])
    if test "$DOXYGEN" = "no"; then
       enable_docs="no"
    fi
fi
AM_CONDITIONAL(ENABLE_DOCS,
    [test "$enable_docs" = "yes"])

AC_SUBST([AM_CFLAGS], ["-g -O2 -Wall -Wno-unused-function -Wno-cpp -Werror"])
AC_SUBST([AM_CXXFLAGS], ["-g -O2 -Wall -Wno-unused-function -Wno-cpp -Wno-missing-braces -Werror"])

# Checks for programs.
AC_DISABLE_STATIC
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
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
VA_API_VERSION=va_min_api_version
AS_IF([test x$enable_h265dec = xyes], [VA_API_VERSION="0.37"])
AS_IF([test x$enable_vp9dec = xyes], [VA_API_VERSION="0.37"])
AS_IF([test x$enable_jpegenc = xyes], [VA_API_VERSION="0.37"])
AS_IF([test x$enable_h265enc = xyes], [VA_API_VERSION="0.38"])
AS_IF([test x$enable_vp8enc = xyes], [VA_API_VERSION="0.38"])
AS_IF([test x$enable_vp9enc = xyes], [VA_API_VERSION="0.39"])
LIBVA_PKG_REQ="libva >= $VA_API_VERSION"
PKG_CHECK_MODULES([LIBVA], [$LIBVA_PKG_REQ])
AC_SUBST(LIBVA_VERSION)
AC_SUBST(LIBVA_PKG_REQ)

LIBVA_DRM_PKG_REQ="libva-drm"
PKG_CHECK_MODULES([LIBVA_DRM], [$LIBVA_DRM_PKG_REQ])
AC_SUBST(LIBVA_DRM_PKG_REQ)

LIBVA_X11_PKG_REQ=""
X11_PKG_REQ=""
if test "$enable_x11" = "yes"; then
    LIBVA_X11_PKG_REQ="libva-x11"
    PKG_CHECK_MODULES([LIBVA_X11], [$LIBVA_X11_PKG_REQ],
        [AC_DEFINE([HAVE_VA_X11], [1],
            [Defined to 1 if VA/X11 API is enabled])])
    X11_PKG_REQ="x11"
    PKG_CHECK_MODULES([X11], [$X11_PKG_REQ])
fi
AC_SUBST(LIBVA_X11_PKG_REQ)
AC_SUBST(X11_PKG_REQ)
if test "$enable_wayland" = "yes"; then
    PKG_CHECK_MODULES([LIBVA_WAYLAND], [libva-wayland],
        [AC_DEFINE([HAVE_VA_WAYLAND], [1],
            [Defined to 1 if VA/WAYLAND API is enabled])],
        [])
fi

if test "$enable_v4l2" = "yes"; then
    PKG_CHECK_MODULES([LIBV4L2], [libv4l2])
    PKG_CHECK_MODULES([LIBEGL], [egl])
    PKG_CHECK_MODULES([LIBGLES2], [glesv2])
fi

# drm_fourcc.h for dma_buf support
if test "$enable_dmabuf" = "yes"; then
    PKG_CHECK_MODULES([LIBDRM], [libdrm])
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
                 vpp/Makefile
                 v4l2/Makefile
                 capi/Makefile
                 doc/Makefile
                 pkgconfig/Makefile])

AC_OUTPUT([
         pkgconfig/libyami.pc
         pkgconfig/libyami_v4l2.pc
         common/YamiVersion.h
])

# Print a configuration summary
DECODERS=""
AS_IF([test x$enable_h265dec = xyes], [DECODERS="$DECODERS h265"])
AS_IF([test x$enable_vc1dec = xyes], [DECODERS="$DECODERS vc1"])
AS_IF([test x$enable_h264dec = xyes], [DECODERS="$DECODERS h264"])
AS_IF([test x$enable_jpegdec = xyes], [DECODERS="$DECODERS jpeg"])
AS_IF([test x$enable_mpeg2dec = xyes], [DECODERS="$DECODERS mpeg2"])
AS_IF([test x$enable_vp8dec = xyes], [DECODERS="$DECODERS vp8"])
AS_IF([test x$enable_vp9dec = xyes], [DECODERS="$DECODERS vp9"])
AS_IF([test x$enable_fakedec = xyes], [DECODERS="$DECODERS fake"])

ENCODERS=""
AS_IF([test x$enable_h265enc = xyes], [ENCODERS="$ENCODERS h265"])
AS_IF([test x$enable_h264enc = xyes], [ENCODERS="$ENCODERS h264"])
AS_IF([test x$enable_jpegenc = xyes], [ENCODERS="$ENCODERS jpeg"])
AS_IF([test x$enable_vp8enc = xyes], [ENCODERS="$ENCODERS vp8"])
AS_IF([test x$enable_vp9enc = xyes], [ENCODERS="$ENCODERS vp9"])

VPPS=" scaler"
AS_IF([test x$enable_oclfilters = xyes], [VPPS="$VPPS oclfilters"])

AC_MSG_RESULT([
    libyami - libyami_version (API: yami_api_version)

    Build decoders ................... :$DECODERS
    Build encoders ....................:$ENCODERS
    Build vpps ........................:$VPPS
    Build gtest unit tests ........... : $HAVE_GTEST
    Build documentation .............. : $enable_docs
    Enable debug ..................... : $enable_debug
    Installation prefix .............. : $prefix
])
