libyami_decoder_source_c = \
	vaapidecoder_base.cpp \
	vaapidecoder_host.cpp \
	vaapidecsurfacepool.cpp \
	vaapidecpicture.cpp \
	$(NULL)

if BUILD_MPEG2_DECODER
libyami_decoder_source_c += vaapidecoder_mpeg2.cpp
endif

if BUILD_H264_DECODER
libyami_decoder_source_c += vaapidecoder_h264.cpp
endif

if BUILD_VP8_DECODER
libyami_decoder_source_c += vaapidecoder_vp8.cpp
endif

if BUILD_H265_DECODER
libyami_decoder_source_c += vaapidecoder_h265.cpp
endif

if BUILD_VP9_DECODER
libyami_decoder_source_c += vaapidecoder_vp9.cpp
endif

if BUILD_VC1_DECODER
libyami_decoder_source_c += vaapidecoder_vc1.cpp
endif

if BUILD_JPEG_DECODER
libyami_decoder_source_c += vaapiDecoderJPEG.cpp
endif

if BUILD_FAKE_DECODER
libyami_decoder_source_c += vaapidecoder_fake.cpp
endif

libyami_decoder_source_h = \
	../interface/VideoCommonDefs.h \
	../interface/VideoDecoderDefs.h \
	../interface/VideoDecoderInterface.h \
	../interface/VideoDecoderHost.h \
	$(NULL)

libyami_decoder_source_h_priv = \
	vaapidecoder_base.h \
	vaapidecsurfacepool.h \
	vaapidecpicture.h \
	$(NULL)

if BUILD_MPEG2_DECODER
libyami_decoder_source_h_priv += vaapidecoder_mpeg2.h
endif

if BUILD_H264_DECODER
libyami_decoder_source_h_priv += vaapidecoder_h264.h
endif

if BUILD_VP8_DECODER
libyami_decoder_source_h_priv += vaapidecoder_vp8.h
endif

if BUILD_H265_DECODER
libyami_decoder_source_h_priv += vaapidecoder_h265.h
endif

if BUILD_VP9_DECODER
libyami_decoder_source_h_priv += vaapidecoder_vp9.h
endif

if BUILD_VC1_DECODER
libyami_decoder_source_h_priv += vaapidecoder_vc1.h
endif

if BUILD_JPEG_DECODER
libyami_decoder_source_h_priv += vaapiDecoderJPEG.h
endif

if BUILD_FAKE_DECODER
libyami_decoder_source_h_priv += vaapidecoder_fake.h
endif

libyami_decoder_ldflags = \
	$(LIBYAMI_LT_LDFLAGS) \
	$(LIBVA_LIBS) \
	$(LIBVA_DRM_LIBS) \
	-ldl \
	$(NULL)

if ENABLE_X11
libyami_decoder_ldflags += $(LIBVA_X11_LIBS) $(X11_LIBS)
endif

#to compile within yocto
extra_includes = \
        -I$(top_srcdir) \
        $(NULL)

libyami_decoder_cppflags = \
	$(LIBVA_CFLAGS) \
	$(LIBVA_DRM_CFLAGS) \
	-I$(top_srcdir)/interface \
	$(extra_includes)  \
	$(NULL)

if ENABLE_X11
libyami_decoder_cppflags += $(LIBVA_X11_CFLAGS)
endif

noinst_LTLIBRARIES             = libyami_decoder.la
libyami_decoderincludedir      = $(includedir)/libyami
libyami_decoderinclude_HEADERS = $(libyami_decoder_source_h)
noinst_HEADERS                 = $(libyami_decoder_source_h_priv)
libyami_decoder_la_SOURCES     = $(libyami_decoder_source_c)
libyami_decoder_la_LDFLAGS     = $(libyami_decoder_ldflags) $(AM_LDFLAGS)
libyami_decoder_la_CPPFLAGS    = $(libyami_decoder_cppflags) $(AM_CPPFLAGS)

if ENABLE_TESTS
include Makefile.unittest
endif

DISTCLEANFILES = \
	Makefile.in 


