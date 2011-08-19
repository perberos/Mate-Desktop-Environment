modules_flags = -export_dynamic -avoid-version -module

plugindir = $(PLUGINDIR)/properties
plugin_LTLIBRARIES = libmovie-properties.la

noinst_LTLIBRARIES = libbaconvideowidgetproperties.la

plugin_in_files = movie-properties.totem-plugin.in

%.totem-plugin: %.totem-plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugin_DATA = $(plugin_in_files:.totem-plugin.in=.totem-plugin)

common_defines = \
	-D_REENTRANT					\
	-DDBUS_API_SUBJECT_TO_CHANGE			\
	-DMATELOCALEDIR=\""$(datadir)/locale"\"	\
	-DMATECONF_PREFIX=\""/apps/totem"\"		\
	-DDATADIR=\""$(datadir)"\"			\
	-DLIBEXECDIR=\""$(libexecdir)"\"		\
	-DBINDIR=\""$(bindir)"\"			\
	-DTOTEM_PLUGIN_DIR=\""$(libdir)/totem/plugins"\"\
	-DG_LOG_DOMAIN=\""Totem"\"			\
	$(DISABLE_DEPRECATED)

libmovie_properties_la_SOURCES = totem-movie-properties.c
libmovie_properties_la_LDFLAGS = $(modules_flags)
libmovie_properties_la_LIBADD = libbaconvideowidgetproperties.la
libmovie_properties_la_CPPFLAGS = $(common_defines)

libmovie_properties_la_CFLAGS = \
	$(DEPENDENCY_CFLAGS)	\
	$(WARN_CFLAGS)		\
	$(DBUS_CFLAGS)		\
	$(AM_CFLAGS)		\
	-I$(top_srcdir)/	\
	-I$(top_srcdir)/src	\
	-I$(top_srcdir)/src/plugins

libbaconvideowidgetproperties_la_SOURCES =		\
	bacon-video-widget-properties.c			\
	bacon-video-widget-properties.h

libbaconvideowidgetproperties_la_CPPFLAGS = $(common_defines)
libbaconvideowidgetproperties_la_CFLAGS =	\
	$(DEPENDENCY_CFLAGS)			\
	$(WARN_CFLAGS)				\
	$(DBUS_CFLAGS)				\
	$(AM_CFLAGS)				\
	-I$(top_srcdir)/			\
	-I$(top_srcdir)/src			\
	-I$(top_srcdir)/src/backend		\
	-I$(top_srcdir)/src/plugins

EXTRA_DIST = $(plugin_in_files)

CLEANFILES = $(plugin_DATA) $(BUILT_SOURCES)
DISTCLEANFILES = $(plugin_DATA)

