INCLUDES =							\
	-I. -I$(srcdir) 					\
	$(GLIB_CFLAGS)						\
	$(DBUS_CFLAGS)						\
	$(MATE_CFLAGS)						\
	$(PANEL_CFLAGS)						\
	$(LIBMATENOTIFY_CFLAGS)					\
	$(GTKUNIQUE_CFLAGS)					\
	-DBINDIR=\"$(bindir)\"			 		\
	-DMATELOCALEDIR=\""$(datadir)/locale"\"	 	\
	-DDATADIR=\"$(datadir)\"				\
	-DPREFIX=\""$(prefix)"\" 				\
	-DSYSCONFDIR=\""$(sysconfdir)"\" 			\
	-DLIBDIR=\""$(libdir)"\" 				\
	-DVERSION="\"$(VERSION)\"" 				\
	-DGPM_DATA=\"$(pkgdatadir)\"				\
	-DEGG_LOG_FILE=\""~/mate-power-manager.log"\"		\
	-DEGG_VERBOSE="\"GPM_VERBOSE\""				\
	-DEGG_LOGGING="\"GPM_LOGGING\""				\
	-DEGG_CONSOLE="\"GPM_CONSOLE\""				\
	-I$(top_srcdir)/libdbus-glib				\
	$(NULL)

libexec_PROGRAMS=mate-brightness-applet

mate_brightness_applet_SOURCES =				\
	brightness-applet.c					\
	egg-dbus-monitor.c					\
	egg-dbus-monitor.h					\
	egg-debug.c						\
	egg-debug.h						\
	gpm-common.c						\
	gpm-common.h

mate_brightness_applet_LDADD =					\
	$(DBUS_LIBS)						\
	$(MATE_LIBS)						\
	$(PANEL_LIBS)

serverdir       = $(libdir)/matecomponent/servers
server_in_files = MATE_BrightnessApplet.server.in
server_DATA     = $(server_in_files:.server.in=.server)

$(server_in_files): $(server_in_files:.server.in=.server.in.in)
	sed -e "s|\@LIBEXECDIR\@|$(libexecdir)|" -e "s|\@VERSION\@|$(VERSION)|" $< > $@

@INTLTOOL_SERVER_RULE@

uidir   = $(datadir)/mate-2.0/ui
ui_DATA = MATE_BrightnessApplet.xml

EXTRA_DIST = MATE_BrightnessApplet.server.in.in $(ui_DATA)

DISTCLEANFILES =						\
	$(server_DATA) $(server_DATA).in
