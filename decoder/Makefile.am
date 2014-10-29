INCLUDES = -I$(top_srcdir) \
           -I$(top_srcdir)/interface \
           -I$(top_srcdir)/common \
           -I$(top_srcdir)/vaapi \
           -I$(top_srcdir)/codecparsers 

libyami_decoder_source_c = \
        vaapidecoder_base.cpp \
        vaapidecoder_host.cpp \
        vaapidecsurfacepool.cpp \
        vaapidecpicture.cpp \
	$(NULL)

if BUILD_H264_DECODER
        libyami_decoder_source_c += vaapidecoder_h264.cpp
        libyami_decoder_source_c += vaapidecoder_h264_dpb.cpp
endif

if BUILD_VP8_DECODER
        libyami_decoder_source_c += vaapidecoder_vp8.cpp
endif

if BUILD_JPEG_DECODER
        libyami_decoder_source_c += vaapidecoder_jpeg.cpp
endif

libyami_decoder_source_h = \
        ../interface/VideoCommonDefs.h      \
        ../interface/VideoDecoderDefs.h      \
        ../interface/VideoDecoderInterface.h \
        ../interface/VideoDecoderHost.h \
	$(NULL)

libyami_decoder_source_h_priv = \
        vaapidecoder_base.h \
        vaapidecsurfacepool.h \
        vaapidecpicture.h \
	$(NULL)

if BUILD_H264_DECODER
        libyami_decoder_source_h_priv += vaapidecoder_h264.h
endif

if BUILD_VP8_DECODER
        libyami_decoder_source_h_priv += vaapidecoder_vp8.h
endif

if BUILD_JPEG_DECODER
        libyami_decoder_source_h_priv += vaapidecoder_jpeg.h
endif

libyami_decoder_la_LIBADD = \
		$(top_builddir)/common/libyami_common.la \
		$(top_builddir)/vaapi/libyami_vaapi.la \
		$(top_builddir)/codecparsers/libcodecparser.la

libyami_decoder_ldflags = \
        $(LIBYAMI_LT_LDFLAGS) \
        -ldl                 \
        $(NULL)

libyami_decoder_cppflags = \
        $(LIBVA_CFLAGS) \
        $(LIBVA_DRM_CFLAGS) \
        -fpermissive \
	$(NULL)
if ENABLE_X11
libyami_decoder_cppflags += $(LIBVA_X11_CFLAGS)
endif


lib_LTLIBRARIES	               = libyami_decoder.la
libyami_decoderincludedir      = $(includedir)/libyami_decoder
libyami_decoderinclude_HEADERS = $(libyami_decoder_source_h)
noinst_HEADERS                 = $(libyami_decoder_source_h_priv)
libyami_decoder_la_SOURCES     = $(libyami_decoder_source_c)
libyami_decoder_la_LDFLAGS     = $(libyami_decoder_ldflags)
libyami_decoder_la_CPPFLAGS    = $(libyami_decoder_cppflags)

DISTCLEANFILES = \
	Makefile.in 


