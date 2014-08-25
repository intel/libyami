INCLUDES = -I$(top_srcdir) \
           -I$(top_srcdir)/interface \
           -I$(top_srcdir)/common \
           -I$(top_srcdir)/vaapi \
           -I$(top_srcdir)/codecparsers

libyami_encoder_source_c = \
        vaapicodedbuffer.cpp \
        vaapiencpicture.cpp \
        vaapiencoder_base.cpp \
        vaapiencoder_host.cpp \
	$(NULL)

if BUILD_H264_ENCODER
        libyami_encoder_source_c += vaapiencoder_h264.cpp
endif

libyami_encoder_source_h = \
        ../interface/VideoCommonDefs.h      \
        ../interface/VideoEncoderDefs.h      \
        ../interface/VideoEncoderInterface.h \
        ../interface/VideoEncoderHost.h \
	$(NULL)

libyami_encoder_source_h_priv = \
        vaapicodedbuffer.h \
        vaapiencpicture.h \
        vaapiencoder_base.h \
	$(NULL)

if BUILD_H264_ENCODER
        libyami_encoder_source_h_priv += vaapiencoder_h264.h
endif

libyami_encoder_la_LIBADD = \
		$(top_builddir)/common/libyami_common.la \
		$(top_builddir)/vaapi/libyami_vaapi.la \
		$(top_builddir)/codecparsers/libcodecparser.la

libyami_encoder_ldflags = \
        $(LIBYAMI_LT_LDFLAGS) \
        $(LIBVA_LIBS)        \
        $(LIBVA_X11_LIBS)    \
        -ldl                 \
        $(NULL)

libyami_encoder_cppflags = \
        $(LIBVA_CFLAGS) \
        $(LIBVA_X11_CFLAGS) \
        -fpermissive \
	$(NULL)


lib_LTLIBRARIES			  = libyami_encoder.la
libyami_encoderincludedir      = $(includedir)/libyami_encoder
libyami_encoderinclude_HEADERS = $(libyami_encoder_source_h)
noinst_HEADERS                   = $(libyami_encoder_source_h_priv)
libyami_encoder_la_SOURCES	  = $(libyami_encoder_source_c)
libyami_encoder_la_LDFLAGS	  = $(libyami_encoder_ldflags)
libyami_encoder_la_CPPFLAGS    = $(libyami_encoder_cppflags)

DISTCLEANFILES = \
	Makefile.in
