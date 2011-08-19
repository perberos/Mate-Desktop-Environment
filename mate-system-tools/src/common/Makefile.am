include $(top_srcdir)/DirsMakefile

noinst_LIBRARIES = libsetuptool.a

INCLUDES += \
	$(GST_CFLAGS) \
	-DDATADIR=\""$(datadir)"\" \
	-DLIBDIR=\""$(libdir)"\" \
	-DSYSCONFDIR=\""$(sysconfdir)"\" \
	-DPREFIX=\""$(prefix)"\" \
	-DMATELOCALEDIR=\""$(datadir)/locale"\"

libsetuptool_a_SOURCES = \
	gst-dialog.c		gst-dialog.h \
	gst-tool.c		gst-tool.h \
	gst-platform-dialog.c	gst-platform-dialog.h \
	gst-filter.c		gst-filter.h \
	gst-service-role.c	gst-service-role.h \
	gst.h

EXTRA_DIST = CommonMakefile $(headers)

-include $(top_srcdir)/git.mk
