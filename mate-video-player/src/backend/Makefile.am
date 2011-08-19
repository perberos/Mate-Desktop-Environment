noinst_PROGRAMS = bvw-test

noinst_LTLIBRARIES = libbaconvideowidget.la

bvw_test_SOURCES = bvw-test.c

bvw_test_CPPFLAGS = \
	-DDATADIR=\"$(pkgdatadir)\"	\
	-DLOGO_PATH=DATADIR\"\"G_DIR_SEPARATOR_S\"totem\"G_DIR_SEPARATOR_S\"totem_logo.png\"
	$(DISABLE_DEPRECATED)		\
	$(AM_CPPFLAGS)

bvw_test_CFLAGS = \
	$(DEPENDENCY_CFLAGS)	\
	$(AM_CFLAGS)		\
	-I$(top_srcdir)/

bvw_test_LDADD = \
	libbaconvideowidget.la \
	$(DEPENDENCY_LIBS) $(XVIDMODE_LIBS) $(XTEST_LIBS)

BVWMARSHALFILES = baconvideowidget-marshal.c baconvideowidget-marshal.h
GLIB_GENMARSHAL=`pkg-config --variable=glib_genmarshal glib-2.0`
BUILT_SOURCES = $(BVWMARSHALFILES)

baconvideowidget-marshal.h: baconvideowidget-marshal.list
	( $(GLIB_GENMARSHAL) --prefix=baconvideowidget_marshal $(srcdir)/baconvideowidget-marshal.list --header > baconvideowidget-marshal.h )
baconvideowidget-marshal.c: baconvideowidget-marshal.h
	( $(GLIB_GENMARSHAL) --prefix=baconvideowidget_marshal $(srcdir)/baconvideowidget-marshal.list --body --header > baconvideowidget-marshal.c )

libbaconvideowidget_la_SOURCES = \
	$(BVWMARSHALFILES) 		\
	bacon-video-widget.h		\
	bacon-resize.h bacon-resize.c 	\
	video-utils.c video-utils.h 	\
	bacon-video-widget-gst-0.10.c			\
	bacon-video-widget-gst-missing-plugins.c	\
	bacon-video-widget-gst-missing-plugins.h	\
	gstscreenshot.c					\
	gstscreenshot.h

libbaconvideowidget_la_CPPFLAGS =		\
	-D_REENTRANT				\
	-DMATECONF_PREFIX=\""/apps/totem"\"	\
	$(DISABLE_DEPRECATED)			\
	$(AM_CPPFLAGS)

libbaconvideowidget_la_CFLAGS =		\
	$(DEPENDENCY_CFLAGS)		\
	$(MM_CFLAGS)			\
	$(MISSING_PLUGINS_CFLAGS)	\
	$(AM_CFLAGS)			\
	-I$(top_srcdir)/


libbaconvideowidget_la_LIBADD =	\
	$(XVIDMODE_LIBS)	\
	$(MM_LIBS)		\
	$(GST_LIBS)		\
	$(EXTRA_BACKEND_LIBS)	\
	$(X_LIBS)		\
	$(MISSING_PLUGINS_LIBS)	\
	-lm

libbaconvideowidget_la_LDFLAGS=	\
	-no-undefined

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
	baconvideowidget-marshal.list
