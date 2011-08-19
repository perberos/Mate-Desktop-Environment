## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_builddir)				\
	-I$(top_srcdir)					\
	-DG_LOG_DOMAIN=\"MateUI\"			\
	$(WARN_CFLAGS)					\
	$(LIBMATEUI_GLADE_CFLAGS)

glademoduledir = $(libdir)/libglade/2.0
glademodule_LTLIBRARIES = libmate.la

libmate_la_LIBADD = \
	$(top_builddir)/libmateui/libmateui-2.la	\
	$(LIBMATEUI_GLADE_LIBS)

libmate_la_LDFLAGS = \
	-export-dynamic -module -avoid-version -no-undefined

libmate_la_SOURCES = \
	glade-mate.c
