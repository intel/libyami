INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/tests

bin_PROGRAMS =
if ENABLE_X11
bin_PROGRAMS += simpleplayer grid
endif

AM_CFLAGS = \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/interface	\
	$(LIBVA_CFLAGS)             \
	$(NULL)

DECODE_INPUT_SOURCES = \
	$(top_builddir)/tests/decodeinput.cpp \
	$(NULL)

if ENABLE_AVFORMAT
DECODE_INPUT_SOURCES += $(top_builddir)/tests/decodeinputavformat.cpp
AM_CFLAGS += -D__STDC_CONSTANT_MACROS \
			 $(LIBAVFORMAT_CFLAGS)
endif

VPP_INPUT_SOURCES = \
	$(DECODE_INPUT_SOURCES) \
	$(top_builddir)/tests/vppinputoutput.cpp \
	$(top_builddir)/tests/vppinputdecode.cpp \
	$(top_builddir)/tests/vppoutputencode.cpp \
	$(top_builddir)/tests/encodeinput.cpp \
	$(top_builddir)/tests/encodeInputDecoder.cpp \
	$(top_builddir)/tests/encodeInputCamera.cpp \
	$(NULL)

AM_CPPFLAGS = $(AM_CFLAGS)
YAMI_COMMON_LIBS 	= \
	$(top_builddir)/common/libyami_common.la	\
	$(top_builddir)/vaapi/libyami_vaapi.la		\
	$(NULL)

if ENABLE_X11
YAMI_COMMON_LIBS += $(LIBVA_X11_LIBS) -lX11
endif

YAMI_DECODE_LIBS 	= \
	$(YAMI_COMMON_LIBS)                            	\
	$(top_builddir)/decoder/libyami_decoder.la      \
	$(NULL)

if ENABLE_TESTS_GLES
YAMI_DECODE_LIBS += $(LIBEGL_LIBS) $(LIBGLES2_LIBS)
endif

if ENABLE_AVFORMAT
YAMI_DECODE_LIBS += $(LIBAVFORMAT_LIBS)
endif

YAMI_ENCODE_LIBS = \
	$(YAMI_COMMON_LIBS)                             \
	$(top_builddir)/encoder/libyami_encoder.la      \
	$(YAMI_DECODE_LIBS)								\
	$(NULL)

VPP_INPUT_LIBS = \
    $(YAMI_COMMON_LIBS)   \
    $(top_builddir)/vpp/libyami_vpp.la	\
    $(LIBYAMI_LT_LDFLAGS) \
    $(LIBVA_LIBS) \
    $(LIBVA_DRM_LIBS) \
    $(YAMI_ENCODE_LIBS) \
	$(NULL)

VPP_INPUT_CFLAGS = \
    $(LIBVA_CFLAGS) \
    -fpermissive \
	$(NULL)

if ENABLE_X11
simpleplayer_LDADD = $(YAMI_DECODE_LIBS)
simpleplayer_SOURCES	= simpleplayer.cpp $(DECODE_INPUT_SOURCES)

grid_LDADD = $(VPP_INPUT_LIBS)
grid_SOURCES = grid.cpp $(VPP_INPUT_SOURCES)
endif
