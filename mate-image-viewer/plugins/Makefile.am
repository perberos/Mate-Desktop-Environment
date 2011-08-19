AUTOMAKE_OPTIONS = subdir-objects

pluginsdir = $(libdir)/eog/plugins

INCLUDES = \
	-I$(top_srcdir)/src 				\
	$(EOG_CFLAGS) 					\
	$(WARN_CFLAGS)					\
	-DEOG_LOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\"

AM_LDFLAGS = -avoid-version -module

plugins_LTLIBRARIES = \
	fullscreen/libfullscreen.la  \
	reload/libreload.la

plugins_in_files = \
	fullscreen/fullscreen.eog-plugin.desktop.in  \
	reload/reload.eog-plugin.desktop.in

# Fullscreen plugin
fullscreen_libfullscreen_la_SOURCES = \
	fullscreen/eog-fullscreen-plugin.h  \
	fullscreen/eog-fullscreen-plugin.c

fullscreen_libfullscreen_la_LIBADD = $(EOG_LIBS)

# Reload plugin
reload_libreload_la_SOURCES = \
	reload/eog-reload-plugin.h  \
	reload/eog-reload-plugin.c

reload_libreload_la_LIBADD = $(EOG_LIBS)

# Statusbar Date Plugin
if HAVE_EXIF
plugins_LTLIBRARIES += statusbar-date/libstatusbar-date.la

statusbar_date_libstatusbar_date_la_SOURCES = \
	statusbar-date/eog-statusbar-date-plugin.h  \
	statusbar-date/eog-statusbar-date-plugin.c

statusbar_date_libstatusbar_date_la_LIBADD = $(EOG_LIBS)
plugins_in_files += statusbar-date/statusbar-date.eog-plugin.desktop.in
endif

# Generate plugin info files

%.eog-plugin: %.eog-plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(AM_V_GEN)$(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugins_DATA = $(plugins_in_files:.eog-plugin.desktop.in=.eog-plugin)

DISTCLEANFILES = $(plugins_DATA)
CLEANFILES = $(plugins_DATA)

EXTRA_DIST = $(plugins_in_files)

-include $(top_srcdir)/git.mk
