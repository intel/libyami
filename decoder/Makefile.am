INCLUDES = -I$(top_srcdir) \
           -I$(top_srcdir)/interface \
           -I$(top_srcdir)/common \
           -I$(top_srcdir)/codecparsers 

libyami_decoder_source_c = \
        vaapipicture.cpp \
        vaapidecoder_base.cpp \
        vaapidecoder_h264.cpp \
        vaapidecoder_h264_dpb.cpp \
        vaapidecoder_vp8.cpp \
        vaapidecoder_jpeg.cpp \
        vaapidecoder_host.cpp \
        vaapisurfacebuf_pool.cpp \
	$(NULL)

libyami_decoder_source_h = \
        ../interface/VideoDecoderDefs.h      \
        ../interface/VideoDecoderInterface.h \
        ../interface/VideoDecoderHost.h \
	$(NULL)

libyami_decoder_source_h_priv = \
        vaapipicture.h    \
        vaapidecoder_base.h \
        vaapidecoder_h264.h \
        vaapidecoder_vp8.h \
        vaapidecoder_jpeg.h \
        vaapisurfacebuf_pool.h \
	$(NULL)

libyami_decoder_la_LIBADD = \
		$(top_builddir)/common/libyami_common.la \
		$(top_builddir)/codecparsers/libcodecparser.la

libyami_decoder_ldflags = \
	-version-info 0:2:0  \
        $(LIBVA_LIBS)        \
        $(LIBVA_X11_LIBS)    \
        -ldl                 \
        $(NULL)

libyami_decoder_cppflags = \
        $(LIBVA_CFLAGS) \
        $(LIBVA_X11_CFLAGS) \
        $(LIBVA_DRM_CFLAGS) \
        -fpermissive \
	$(NULL)


lib_LTLIBRARIES	               = libyami_decoder.la
libyami_decoderincludedir      = $(includedir)/libyami_decoder
libyami_decoderinclude_HEADERS = $(libyami_decoder_source_h)
noinst_HEADERS                 = $(libyami_decoder_source_h_priv)
libyami_decoder_la_SOURCES     = $(libyami_decoder_source_c)
libyami_decoder_la_LDFLAGS     = $(libyami_decoder_ldflags)
libyami_decoder_la_CPPFLAGS    = $(libyami_decoder_cppflags)

DISTCLEANFILES = \
	Makefile.in 


