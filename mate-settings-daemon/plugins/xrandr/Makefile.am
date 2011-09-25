icondir = $(datadir)/icons/mate
context = apps

BUILT_SOURCES =				\
	gsd-xrandr-manager-glue.h

ICON_FILES = 			\
	gsd-xrandr-16.png	\
	gsd-xrandr-22.png	\
	gsd-xrandr-24.png	\
	gsd-xrandr-32.png	\
	gsd-xrandr.svg

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(sysconfdir)/mate-settings-daemon/xrandr
	$(mkinstalldirs) $(DESTDIR)$(icondir)/16x16/$(context)
	$(mkinstalldirs) $(DESTDIR)$(icondir)/22x22/$(context)
	$(mkinstalldirs) $(DESTDIR)$(icondir)/24x24/$(context)
	$(mkinstalldirs) $(DESTDIR)$(icondir)/32x32/$(context)
	$(mkinstalldirs) $(DESTDIR)$(icondir)/scalable/$(context)
	$(INSTALL_DATA) $(srcdir)/gsd-xrandr-16.png $(DESTDIR)$(icondir)/16x16/$(context)/gsd-xrandr.png
	$(INSTALL_DATA) $(srcdir)/gsd-xrandr-22.png $(DESTDIR)$(icondir)/22x22/$(context)/gsd-xrandr.png
	$(INSTALL_DATA) $(srcdir)/gsd-xrandr-24.png $(DESTDIR)$(icondir)/24x24/$(context)/gsd-xrandr.png
	$(INSTALL_DATA) $(srcdir)/gsd-xrandr-32.png $(DESTDIR)$(icondir)/32x32/$(context)/gsd-xrandr.png
	$(INSTALL_DATA) $(srcdir)/gsd-xrandr.svg $(DESTDIR)$(icondir)/scalable/$(context)/gsd-xrandr.svg

uninstall-local:
	rm -f $(DESTDIR)$(icondir)/16x16/$(context)/gsd-xrandr.png
	rm -f $(DESTDIR)$(icondir)/22x22/$(context)/gsd-xrandr.png
	rm -f $(DESTDIR)$(icondir)/24x24/$(context)/gsd-xrandr.png
	rm -f $(DESTDIR)$(icondir)/32x32/$(context)/gsd-xrandr.png
	rm -f $(DESTDIR)$(icondir)/scalable/$(context)/gsd-xrandr.svg

plugin_LTLIBRARIES = \
	libxrandr.la

gsd-xrandr-manager-glue.h: gsd-xrandr-manager.xml Makefile
	dbus-binding-tool --prefix=gsd_xrandr_manager --mode=glib-server $< > xgen-$(@F) \
	&& ( cmp -s xgen-$(@F) $@ || cp xgen-$(@F) $@ ) \
	&& rm -f xgen-$(@F)

libxrandr_la_SOURCES = 		\
	$(BUILT_SOURCES)	\
	gsd-xrandr-plugin.h	\
	gsd-xrandr-plugin.c	\
	gsd-xrandr-manager.h	\
	gsd-xrandr-manager.c

libxrandr_la_CPPFLAGS =						\
	-I$(top_srcdir)/mate-settings-daemon			\
	-DBINDIR=\"$(bindir)\"					\
	-DMATE_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\"	\
	$(AM_CPPFLAGS)

libxrandr_la_CFLAGS =			\
	$(SETTINGS_PLUGIN_CFLAGS)	\
	$(LIBMATENOTIFY_CFLAGS)		\
	$(AM_CFLAGS)

libxrandr_la_LDFLAGS = 			\
	$(GSD_PLUGIN_LDFLAGS)

libxrandr_la_LIBADD  =			\
	$(SETTINGS_PLUGIN_LIBS)		\
	$(LIBMATENOTIFY_LIBS)

plugin_in_files =			\
	xrandr.mate-settings-plugin.in

plugin_DATA = $(plugin_in_files:.mate-settings-plugin.in=.mate-settings-plugin)

EXTRA_DIST = $(plugin_in_files) $(ICON_FILES) gsd-xrandr-manager.xml
CLEANFILES = $(plugin_DATA) $(BUILT_SOURCES)
DISTCLEANFILES = $(plugin_DATA)

@GSD_INTLTOOL_PLUGIN_RULE@
