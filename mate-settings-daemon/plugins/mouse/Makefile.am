plugin_LTLIBRARIES = libmouse.la

libmouse_la_SOURCES = 		\
	gsd-mouse-plugin.h	\
	gsd-mouse-plugin.c	\
	gsd-mouse-manager.h	\
	gsd-mouse-manager.c

libmouse_la_CPPFLAGS = \
	-I$(top_srcdir)/mate-settings-daemon		\
	-DMATE_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\" \
	-DLIBEXECDIR=\""$(libexecdir)"\" \
	$(AM_CPPFLAGS)

libmouse_la_CFLAGS = \
	$(SETTINGS_PLUGIN_CFLAGS)	\
	$(AM_CFLAGS)

libmouse_la_LDFLAGS = 		\
	$(GSD_PLUGIN_LDFLAGS)

libmouse_la_LIBADD  = 		\
	$(SETTINGS_PLUGIN_LIBS)	\
	$(X11_LIBS)		\
	$(XINPUT_LIBS)

plugin_in_files = mouse.mate-settings-plugin.in

plugin_DATA = $(plugin_in_files:.mate-settings-plugin.in=.mate-settings-plugin)

libexec_PROGRAMS = gsd-locate-pointer

gsd_locate_pointer_SOURCES = 	\
	gsd-locate-pointer.h	\
	gsd-locate-pointer.c	\
	gsd-timeline.h		\
	gsd-timeline.c

gsd_locate_pointer_CFLAGS = \
	$(SETTINGS_PLUGIN_CFLAGS)	\
	$(AM_CFLAGS)

gsd_locate_pointer_LDADD  = 		\
	$(SETTINGS_PLUGIN_LIBS)		\
	$(X11_LIBS)			\
	-lm

EXTRA_DIST = $(plugin_in_files)
CLEANFILES = $(plugin_DATA)
DISTCLEANFILES = $(plugin_DATA)

@GSD_INTLTOOL_PLUGIN_RULE@
