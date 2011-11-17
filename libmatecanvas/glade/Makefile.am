## Process this file with automake to produce Makefile.in

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

INCLUDES = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"MateCanvas\" \
	$(CANVAS_GLADE_CFLAGS)

glademoduledir = $(libdir)/libglade/2.0

if USE_GLADE
glademodule_LTLIBRARIES = libgladematecanvas.la
endif

libgladematecanvas_la_LIBADD = \
	$(top_builddir)/libmatecanvas/libmatecanvas-2.la \
	$(CANVAS_GLADE_LIBS)

libgladematecanvas_la_LDFLAGS = \
	-export-dynamic -module -avoid-version $(no_undefined)

libgladematecanvas_la_SOURCES = \
	glade-canvas.c
