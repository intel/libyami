libyami_vpp_source_c = \
	vaapipostprocess_base.cpp \
	vaapipostprocess_host.cpp \
	vaapipostprocess_scaler.cpp \
	vaapivpppicture.cpp \
	$(NULL)

if BUILD_OCL_FILTERS
libyami_vpp_source_c += ../ocl/oclcontext.cpp
libyami_vpp_source_c += oclpostprocess_base.cpp
libyami_vpp_source_c += oclvppimage.cpp
endif

if BUILD_OCL_FILTERS
libyami_vpp_source_c += oclpostprocess_blender.cpp
libyami_vpp_source_c += oclpostprocess_osd.cpp
libyami_vpp_source_c += oclpostprocess_transform.cpp
libyami_vpp_source_c += oclpostprocess_mosaic.cpp
libyami_vpp_source_c += oclpostprocess_wireframe.cpp
endif

libyami_vpp_source_h = \
	../interface/VideoCommonDefs.h \
	../interface/VideoPostProcessDefs.h \
	../interface/VideoPostProcessHost.h \
	../interface/VideoPostProcessInterface.h \
	$(NULL)

libyami_vpp_source_h_priv = \
	vaapipostprocess_base.h \
	vaapipostprocess_scaler.h \
	vaapivpppicture.h \
	$(NULL)

if BUILD_OCL_FILTERS
libyami_vpp_source_h_priv += ../ocl/oclcontext.h
libyami_vpp_source_h_priv += oclpostprocess_base.h
libyami_vpp_source_h_priv += oclvppimage.h
endif

if BUILD_OCL_FILTERS
libyami_vpp_source_h_priv += oclpostprocess_blender.h
libyami_vpp_source_h_priv += oclpostprocess_osd.h
libyami_vpp_source_h_priv += oclpostprocess_transform.h
libyami_vpp_source_h_priv += oclpostprocess_mosaic.h
libyami_vpp_source_h_priv += oclpostprocess_wireframe.h
endif

libyami_vpp_ldflags = \
	$(LIBYAMI_LT_LDFLAGS) \
	$(LIBVA_LIBS) \
	$(LIBVA_DRM_LIBS) \
	-ldl \
	$(NULL)

#to compile within yocto
extra_includes = \
        -I$(top_srcdir) \
        $(NULL)

libyami_vpp_cppflags = \
	$(LIBVA_CFLAGS) \
	$(LIBVA_DRM_CFLAGS) \
	-I$(top_srcdir)/interface \
	$(extra_includes) \
	$(NULL)

if BUILD_OCL_FILTERS
libyami_vpp_ocl_kernel = \
	../ocl/kernels/blend.cl \
	../ocl/kernels/osd.cl \
	../ocl/kernels/transform.cl \
	../ocl/kernels/mosaic.cl \
	../ocl/kernels/wireframe.cl \
	$(NULL)
kerneldir             = $(datadir)/libyami/kernel
kernel_DATA           = $(libyami_vpp_ocl_kernel)
libyami_vpp_cppflags += -DKERNEL_DIR=\"$(kerneldir)/\"
endif

if BUILD_OCL_FILTERS
libyami_vpp_ldflags += -lOpenCL
endif

noinst_LTLIBRARIES          = libyami_vpp.la
libyami_vppincludedir       = $(includedir)/libyami
libyami_vppinclude_HEADERS  = $(libyami_vpp_source_h)
noinst_HEADERS              = $(libyami_vpp_source_h_priv)
libyami_vpp_la_SOURCES      = $(libyami_vpp_source_c)
libyami_vpp_la_LDFLAGS      = $(libyami_vpp_ldflags) $(AM_LDFLAGS)
libyami_vpp_la_CPPFLAGS     = $(libyami_vpp_cppflags) $(AM_CPPFLAGS)

if ENABLE_TESTS
include Makefile.unittest
endif

DISTCLEANFILES = \
	Makefile.in
