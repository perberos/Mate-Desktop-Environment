icondir = $(datadir)/icons/mate
context = actions

NULL =

SUBDIRS =
plugin_LTLIBRARIES =

if HAVE_PULSE
SUBDIRS += cut-n-paste
plugin_LTLIBRARIES += libmedia-keys.la
endif

BUILT_SOURCES = 			\
	gsd-media-keys-manager-glue.h	\
	gsd-marshal.h			\
	gsd-marshal.c			\
	$(NULL)

ICON_FILES = 			\
	touchpad-disabled-16.png	\
	touchpad-enabled-16.png		\
	touchpad-disabled-22.png	\
	touchpad-enabled-22.png		\
	touchpad-disabled-24.png	\
	touchpad-enabled-24.png		\
	touchpad-disabled-32.png	\
	touchpad-enabled-32.png		\
	touchpad-disabled-48.png	\
	touchpad-enabled-48.png		\
	touchpad-disabled.svg		\
	touchpad-enabled.svg

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(icondir)/16x16/$(context)
	$(mkinstalldirs) $(DESTDIR)$(icondir)/22x22/$(context)
	$(mkinstalldirs) $(DESTDIR)$(icondir)/24x24/$(context)
	$(mkinstalldirs) $(DESTDIR)$(icondir)/32x32/$(context)
	$(mkinstalldirs) $(DESTDIR)$(icondir)/scalable/$(context)
	$(INSTALL_DATA) $(srcdir)/touchpad-enabled-16.png $(DESTDIR)$(icondir)/16x16/$(context)/touchpad-enabled.png
	$(INSTALL_DATA) $(srcdir)/touchpad-enabled-22.png $(DESTDIR)$(icondir)/22x22/$(context)/touchpad-enabled.png
	$(INSTALL_DATA) $(srcdir)/touchpad-enabled-24.png $(DESTDIR)$(icondir)/24x24/$(context)/touchpad-enabled.png
	$(INSTALL_DATA) $(srcdir)/touchpad-enabled-32.png $(DESTDIR)$(icondir)/32x32/$(context)/touchpad-enabled.png
	$(INSTALL_DATA) $(srcdir)/touchpad-enabled.svg $(DESTDIR)$(icondir)/scalable/$(context)/touchpad-enabled.svg
	$(INSTALL_DATA) $(srcdir)/touchpad-disabled-16.png $(DESTDIR)$(icondir)/16x16/$(context)/touchpad-disabled.png
	$(INSTALL_DATA) $(srcdir)/touchpad-disabled-22.png $(DESTDIR)$(icondir)/22x22/$(context)/touchpad-disabled.png
	$(INSTALL_DATA) $(srcdir)/touchpad-disabled-24.png $(DESTDIR)$(icondir)/24x24/$(context)/touchpad-disabled.png
	$(INSTALL_DATA) $(srcdir)/touchpad-disabled-32.png $(DESTDIR)$(icondir)/32x32/$(context)/touchpad-disabled.png
	$(INSTALL_DATA) $(srcdir)/touchpad-disabled.svg $(DESTDIR)$(icondir)/scalable/$(context)/touchpad-disabled.svg

uninstall-local:
	rm -f $(DESTDIR)$(icondir)/16x16/$(context)/touchpad-enabled.png
	rm -f $(DESTDIR)$(icondir)/22x22/$(context)/touchpad-enabled.png
	rm -f $(DESTDIR)$(icondir)/24x24/$(context)/touchpad-enabled.png
	rm -f $(DESTDIR)$(icondir)/32x32/$(context)/touchpad-enabled.png
	rm -f $(DESTDIR)$(icondir)/scalable/$(context)/touchpad-enabled.svg
	rm -f $(DESTDIR)$(icondir)/16x16/$(context)/touchpad-disabled.png
	rm -f $(DESTDIR)$(icondir)/22x22/$(context)/touchpad-disabled.png
	rm -f $(DESTDIR)$(icondir)/24x24/$(context)/touchpad-disabled.png
	rm -f $(DESTDIR)$(icondir)/32x32/$(context)/touchpad-disabled.png
	rm -f $(DESTDIR)$(icondir)/scalable/$(context)/touchpad-disabled.svg

gsd-media-keys-manager-glue.h: gsd-media-keys-manager.xml Makefile
	dbus-binding-tool --prefix=gsd_media_keys_manager --mode=glib-server $< > xgen-$(@F) \
	&& ( cmp -s xgen-$(@F) $@ || cp xgen-$(@F) $@ ) \
	&& rm -f xgen-$(@F)

gsd-marshal.c: gsd-marshal.list
	$(GLIB_GENMARSHAL) --prefix=gsd_marshal $< --header --body --internal > $@

gsd-marshal.h: gsd-marshal.list
	$(GLIB_GENMARSHAL) --prefix=gsd_marshal $< --header --internal > $@

libmedia_keys_la_SOURCES = 		\
	gsd-media-keys-plugin.h		\
	gsd-media-keys-plugin.c		\
	gsd-media-keys-manager.h	\
	gsd-media-keys-manager.c	\
	gsd-media-keys-window.h		\
	gsd-media-keys-window.c		\
	acme.h				\
	$(BUILT_SOURCES)		\
	$(NULL)

libmedia_keys_la_CPPFLAGS = \
	-I$(top_srcdir)/mate-settings-daemon			\
	-I$(top_srcdir)/plugins/common				\
	-I$(top_srcdir)/plugins/media-keys/cut-n-paste		\
	-DPIXMAPDIR=\""$(pkgdatadir)"\"				\
	-DGTKBUILDERDIR=\""$(pkgdatadir)"\"			\
	-DMATE_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\"	\
	$(AM_CPPFLAGS)

libmedia_keys_la_CFLAGS = \
	$(SETTINGS_PLUGIN_CFLAGS)	\
	$(AM_CFLAGS)

libmedia_keys_la_LDFLAGS = 		\
	$(GSD_PLUGIN_LDFLAGS)

libmedia_keys_la_LIBADD  = 		\
	$(top_builddir)/plugins/common/libcommon.la			\
	$(top_builddir)/plugins/media-keys/cut-n-paste/libgvc.la	\
	$(SETTINGS_PLUGIN_LIBS)						\
	$(XF86MISC_LIBS)						\
	-lm

plugin_in_files = 		\
	media-keys.mate-settings-plugin.in

if HAVE_PULSE
plugin_DATA = $(plugin_in_files:.mate-settings-plugin.in=.mate-settings-plugin)
endif

noinst_PROGRAMS =				\
	test-media-keys				\
	test-media-window			\
	$(NULL)

test_media_window_SOURCES =			\
	gsd-media-keys-window.c			\
	gsd-media-keys-window.h			\
	test-media-window.c			\
	$(NULL)

test_media_window_CPPFLAGS = \
	-I$(top_srcdir)/mate-settings-daemon			\
	-I$(top_srcdir)/plugins/common				\
	-I$(top_srcdir)/plugins/media-keys/cut-n-paste		\
	-DDATADIR=\""$(datadir)"\"				\
	-DPIXMAPDIR=\""$(pkgdatadir)"\"				\
	-DGTKBUILDERDIR=\""$(pkgdatadir)"\"			\
	-DMATE_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\"	\
	$(AM_CPPFLAGS)

test_media_window_CFLAGS = \
	$(SETTINGS_PLUGIN_CFLAGS)	\
	$(AM_CFLAGS)

test_media_window_LDADD = \
	$(top_builddir)/plugins/common/libcommon.la			\
	$(SETTINGS_DAEMON_LIBS)			\
	$(SETTINGS_PLUGIN_LIBS)			\
	$(XF86MISC_LIBS)			\
	$(GST_LIBS)				\
	-lm

test_media_keys_SOURCES =			\
	gsd-media-keys-manager.c		\
	gsd-media-keys-manager.h		\
	gsd-media-keys-window.h			\
	gsd-media-keys-window.c			\
	test-media-keys.c			\
	$(BUILT_SOURCES)			\
	$(NULL)

test_media_keys_CPPFLAGS = \
	-I$(top_srcdir)/mate-settings-daemon			\
	-I$(top_srcdir)/plugins/common				\
	-I$(top_srcdir)/plugins/media-keys/cut-n-paste		\
	-DPIXMAPDIR=\""$(pkgdatadir)"\"				\
	-DGTKBUILDERDIR=\""$(pkgdatadir)"\"			\
	-DMATE_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\"	\
	$(AM_CPPFLAGS)

test_media_keys_CFLAGS = \
	$(SETTINGS_PLUGIN_CFLAGS)	\
	$(AM_CFLAGS)

test_media_keys_LDADD = \
	$(top_builddir)/mate-settings-daemon/libgsd-profile.la		\
	$(top_builddir)/plugins/common/libcommon.la			\
	$(SETTINGS_DAEMON_LIBS)			\
	$(SETTINGS_PLUGIN_LIBS)			\
	$(XF86MISC_LIBS)			\
	$(GST_LIBS)				\
	-lm

if HAVE_PULSE
test_media_keys_LDADD += $(top_builddir)/plugins/media-keys/cut-n-paste/libgvc.la
endif

gtkbuilderdir = $(pkgdatadir)
gtkbuilder_DATA =	\
	acme.ui		\
	$(NULL)

DIST_SUBDIRS = cut-n-paste

EXTRA_DIST = 				\
	gsd-media-keys-manager.xml	\
	gsd-marshal.list		\
	$(plugin_in_files)		\
	$(gtkbuilder_DATA)		\
	$(pixmaps_DATA)			\
	touchpad-enabled-template.svg	\
	touchpad-disabled-template.svg	\
	$(ICON_FILES)

CLEANFILES = 			\
	$(BUILT_SOURCES)	\
	$(plugin_DATA)

DISTCLEANFILES =		\
	$(plugin_DATA)

@GSD_INTLTOOL_PLUGIN_RULE@
