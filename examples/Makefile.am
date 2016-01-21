INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/tests

bin_PROGRAMS =

if ENABLE_X11
bin_PROGRAMS += simpleplayer

if BUILD_OCL_BLENDER
bin_PROGRAMS += blend
endif

endif

if ENABLE_DMABUF
bin_PROGRAMS += grid
endif

AM_CFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/interface \
	$(LIBVA_CFLAGS) \
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
	$(top_builddir)/tests/vppinputdecode.cpp \
	$(top_builddir)/tests/vppinputasync.cpp \
	$(top_builddir)/tests/vppoutputencode.cpp \
	$(top_builddir)/tests/encodeinput.cpp \
	$(top_builddir)/tests/encodeInputDecoder.cpp \
	$(top_builddir)/tests/encodeInputCamera.cpp \
	$(NULL)

AM_CPPFLAGS = $(AM_CFLAGS)
YAMI_COMMON_LIBS = \
	$(LIBVA_LIBS) \
	$(LIBVA_DRM_LIBS) \
	$(top_builddir)/libyami.la \
	$(NULL)

if ENABLE_X11
YAMI_COMMON_LIBS += $(LIBVA_X11_LIBS) -lX11
endif

YAMI_DECODE_LIBS = \
	$(YAMI_COMMON_LIBS) \
	$(NULL)

if BUILD_STATIC
YAMI_DECODE_LDFLAGS = \
	-Wl,--whole-archive \
	-Wl,$(top_builddir)/.libs/libyami.a \
	-Wl,--no-whole-archive
endif

if ENABLE_TESTS_GLES
YAMI_DECODE_LIBS += $(LIBEGL_LIBS) $(LIBGLES2_LIBS)
endif

if ENABLE_AVFORMAT
YAMI_DECODE_LIBS += $(LIBAVFORMAT_LIBS)
endif

YAMI_ENCODE_LIBS = \
	$(YAMI_DECODE_LIBS) \
	$(NULL)

if BUILD_STATIC
YAMI_ENCODE_LDFLAGS = \
	$(YAMI_DECODE_LDFLAGS) \
	$(NULL)
endif

VPP_INPUT_LIBS = \
	$(LIBYAMI_LT_LDFLAGS) \
	$(YAMI_ENCODE_LIBS) \
	$(NULL)

if BUILD_STATIC
VPP_INPUT_LDFLAGS = \
	$(YAMI_ENCODE_LDFLAGS)
endif

VPP_INPUT_CFLAGS = \
	$(LIBVA_CFLAGS) \
	$(NULL)

if ENABLE_X11
simpleplayer_LDADD = $(YAMI_DECODE_LIBS)
simpleplayer_LDFLAGS = $(YAMI_DECODE_LDFLAGS)
simpleplayer_SOURCES = simpleplayer.cpp $(DECODE_INPUT_SOURCES)

blend_LDADD = $(VPP_INPUT_LIBS) -lpthread
blend_LDFLAGS = $(VPP_INPUT_LDFLAGS)
blend_SOURCES	= blend.cpp $(VPP_INPUT_SOURCES)
endif

if ENABLE_DMABUF
grid_CFLAGS = $(LIBDRM_CFLAGS)
grid_CXXFLAGS = $(LIBDRM_CFLAGS)
grid_LDADD = $(VPP_INPUT_LIBS) $(LIBDRM_LIBS) -lpthread
grid_LDFLAGS = $(VPP_INPUT_LDFLAGS)
grid_SOURCES = grid.cpp $(VPP_INPUT_SOURCES)
endif
