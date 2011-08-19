if PLATFORM_WIN32
no_undefined = -no-undefined
endif

INCLUDES = \
	-I$(top_srcdir) 				\
	-I$(top_builddir)				\
	$(WARN_CFLAGS) 					\
	$(LIBMATECANVAS_CFLAGS)			\
	$(LIBMATECANVAS_DEPRECATED_CFLAGS)		\
        -DMATECANVASLIBDIR=\""$(libdir)"\" 		\
        -DMATECANVASDATADIR=\""$(datadir)"\" 		\
        -DMATECANVASPIXMAPDIR=\""$(datadir)/pixmaps"\"	\
        -DMATECANVASBINDIR=\""$(bindir)"\" 		\
        -DMATECANVASLOCALSTATEDIR=\""$(localstatedir)"\" 	\
	-DG_LOG_DOMAIN=\"MateCanvas\"

lib_LTLIBRARIES = libmatecanvas-2.la

libmatecanvas_2_la_LIBADD = \
	$(LIBMATECANVAS_LIBS)

libmatecanvas_2_la_LDFLAGS = $(no_undefined) \
	-version-info $(LIBMATECANVAS_CURRENT):$(LIBMATECANVAS_REVISION):$(LIBMATECANVAS_AGE) 

libmatecanvasincludedir = $(includedir)/libmatecanvas-2.0/libmatecanvas

libmatecanvas_public_h_sources =			\
	mate-canvas-shape.h			\
	mate-canvas-bpath.h			\
	mate-canvas-clipgroup.h		\
	mate-canvas-line.h			\
	mate-canvas-pixbuf.h			\
	mate-canvas-polygon.h			\
	mate-canvas-rect-ellipse.h		\
	mate-canvas-text.h			\
	mate-canvas-rich-text.h		\
	mate-canvas-util.h			\
	mate-canvas-widget.h			\
	mate-canvas-path-def.h			\
	mate-canvas.h				\
	libmatecanvas.h

libmatecanvas_private_h_sources = 	\
	mate-canvas-shape-private.h	\
	gailcanvas.h			\
	gailcanvasitem.h		\
	gailcanvasgroup.h		\
	gailcanvaswidget.h		\
	gailcanvastext.h		\
	gailcanvasitemfactory.h		\
	gailcanvasgroupfactory.h	\
	gailcanvaswidgetfactory.h	\
	gailcanvastextfactory.h

libmatecanvas_c_sources =			\
	mate-canvas-shape.c			\
	mate-canvas-bpath.c			\
	mate-canvas-path-def.c			\
	mate-canvas-clipgroup.c		\
	mate-canvas-i18n.h			\
	mate-canvas-line.c			\
	mate-canvas-pixbuf.c			\
	mate-canvas-polygon.c			\
	mate-canvas-rect-ellipse.c		\
	mate-canvas-text.c			\
	mate-canvas-rich-text.c		\
	mate-canvas-util.c			\
	mate-canvas-widget.c			\
	mate-canvas.c				\
	libmatecanvastypes.c			\
	gailcanvas.c				\
	gailcanvasitem.c			\
	gailcanvasgroup.c			\
	gailcanvaswidget.c			\
	gailcanvastext.c			\
	gailcanvasitemfactory.c			\
	gailcanvasgroupfactory.c		\
	gailcanvaswidgetfactory.c		\
	gailcanvastextfactory.c	

libmatecanvas_2_la_SOURCES =			\
	$(libmatecanvas_c_sources)	\
	$(libmatecanvas_private_h_sources)

libmatecanvasinclude_HEADERS = 	\
	$(libmatecanvas_public_h_sources)

$(libmatecanvas_2_la_OBJECTS): $(marshal_sources)

marshal_sources =				\
	mate-canvas-marshal.c			\
	mate-canvas-marshal.h

BUILT_SOURCES =					\
	$(marshal_sources)

mate-canvas-marshal.h: mate-canvas-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=mate_canvas_marshal > $@
mate-canvas-marshal.c: mate-canvas-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --body --prefix=mate_canvas_marshal > $@

CLEANFILES = $(marshal_sources)

EXTRA_DIST = \
	mate-canvas-marshal.list	\
	libmatecanvas-2.0.pc.in	\
	libmatecanvas-2.0-uninstalled.pc.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libmatecanvas-2.0.pc

DONT_DIST_SOURCE = $(marshal_sources)

dist-hook:
	for file in $(DONT_DIST_SOURCE) ; do \
	    rm -f $(distdir)/$$file ; \
	done
