all: html
install-data-local: install-html

EXTRA_DIST = \
	doc.config \
	$(NULL)

LIBYAMI_DOC_STRIP_DIR = $(top_srcdir)/common $(top_srcdir)/vaapi $(top_srcdir)/decoder $(top_srcdir)/encoder $(top_srcdir)/interface
LIBYAMI_DOC_STRIP_FILES = vaapibuffer.h \
	$(top_srcdir)/vaapi/vaapiimage.h \
	$(top_srcdir)/vaapi/vaapiptrs.h \
	$(top_srcdir)/vaapi/vaapisurface.h \
	$(top_srcdir)/vaapi/vaapiutils.h \
	$(top_srcdir)/decoder/vaapidecoder_base.h \
	$(top_srcdir)/decoder/vaapidecoder_h264.h \
	$(top_srcdir)/decoder/vaapidecoder_jpeg.h \
	$(top_srcdir)/decoder/vaapidecoder_vp8.h \
	$(top_srcdir)/decoder/vaapidecsurfacepool.h \
	$(top_srcdir)/decoder/vaapipicture.h \
	$(top_srcdir)/decoder/vaapisurfacebuf_pool.h \
	$(top_srcdir)/encoder/vaapicodedbuffer.h \
	$(top_srcdir)/encoder/vaapiencoder_base.h \
	$(top_srcdir)/encoder/vaapiencoder_h264.h \
	$(top_srcdir)/encoder/vaapiencpicture.h \
	$(top_srcdir)/interface/VideoDecoderDefs.h \
	$(top_srcdir)/interface/VideoDecoderHost.h \
	$(top_srcdir)/interface/VideoDecoderInterface.h \
	$(top_srcdir)/interface/VideoEncoderAVC.h \
	$(top_srcdir)/interface/VideoEncoderBase.h \
	$(top_srcdir)/interface/VideoEncoderDefs.h \
	$(top_srcdir)/interface/VideoEncoderHost.h \
	$(top_srcdir)/interface/VideoEncoderInterface.h \
	$(NULL)

export LIBYAMI_DOC_STRIP_DIR
export LIBYAMI_DOC_STRIP_FILES
html/index.html: doc.config
	$(DOXYGEN) $<

if ENABLE_DOCS
html: html/index.html
install-html-local:
	install -d $(DESTDIR)$(docdir)/html
	for file in `ls html/` ; do \
	if test -f html/$$file ; then \
		install -m 0644 html/$$file $(DESTDIR)$(docdir)/html ; \
	else \
		install -d $(DESTDIR)$(docdir)/html/$$file ; \
		install -m 0644 html/$$file/* $(DESTDIR)$(docdir)/html/$$file; \
	fi ; \
	done
uninstall-local:
	rm -rf $(DESTDIR)$(docdir)/html
endif

# Extra clean files so that maintainer-clean removes *everything*
MAINTAINERCLEANFILES = Makefile.in
