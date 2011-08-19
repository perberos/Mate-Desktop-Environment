## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_builddir)				\
	-I$(top_srcdir)					\
	-DG_LOG_DOMAIN=\"MateComponentUI\"			\
	$(MATECOMPONENTUI_GLADE_CFLAGS)

glademoduledir = $(libdir)/libglade/2.0
glademodule_LTLIBRARIES = libmatecomponent.la

libmatecomponent_la_LIBADD = \
	$(top_builddir)/matecomponent/libmatecomponentui-2.la	\
	$(MATECOMPONENTUI_GLADE_LIBS)

libmatecomponent_la_LDFLAGS = \
	-export-dynamic -module -avoid-version -no-undefined

libmatecomponent_la_SOURCES = \
	glade-matecomponent.c
