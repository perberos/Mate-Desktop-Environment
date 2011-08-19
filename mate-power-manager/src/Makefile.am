## We require new-style dependency handling.
AUTOMAKE_OPTIONS = 1.7

NULL =

EXTRA_DIST =						\
	org.mate.PowerManager.xml			\
	org.mate.PowerManager.Backlight.xml		\
	gpm-marshal.list				\
	$(NULL)

INCLUDES =						\
	$(GLIB_CFLAGS)					\
	$(DBUS_CFLAGS)					\
	$(MATE_CFLAGS)					\
	$(UNIQUE_CFLAGS)				\
	$(X11_CFLAGS)					\
	$(LIBMATENOTIFY_CFLAGS)				\
	$(CANBERRA_CFLAGS)				\
	$(GSTREAMER_CFLAGS)				\
	-DI_KNOW_THE_DEVICEKIT_POWER_API_IS_SUBJECT_TO_CHANGE \
	$(UPOWER_CFLAGS)				\
	-DBINDIR=\"$(bindir)\"			 	\
	-DSBINDIR=\"$(sbindir)\"			 \
	-DMATELOCALEDIR=\""$(datadir)/locale"\"	\
	-DDATADIR=\"$(datadir)\"			\
	-DPREFIX=\""$(prefix)"\" 			\
	-DSYSCONFDIR=\""$(sysconfdir)"\" 		\
	-DLIBDIR=\""$(libdir)"\" 			\
	-DVERSION="\"$(VERSION)\"" 			\
	-DGPM_DATA=\"$(pkgdatadir)\"			\
	-DGTKBUILDERDIR=\"$(pkgdatadir)\"		\
	-DUP_DISABLE_DEPRECATED			\
	-DEGG_LOG_FILE=\""/tmp/gpm.log"\"		\
	-DEGG_VERBOSE="\"GPM_VERBOSE\""			\
	-DEGG_LOGGING="\"GPM_LOGGING\""			\
	-DEGG_CONSOLE="\"GPM_CONSOLE\""			\
	-I$(top_srcdir)					\
	$(NULL)

bin_PROGRAMS =						\
	mate-power-manager				\
	mate-power-preferences				\
	mate-power-statistics				\
	$(NULL)

sbin_PROGRAMS =						\
	mate-power-backlight-helper			\
	$(NULL)

check_PROGRAMS =					\
	mate-power-self-test

noinst_LIBRARIES = libgpmshared.a
libgpmshared_a_SOURCES =				\
	egg-color.c					\
	egg-color.h					\
	egg-debug.h					\
	egg-debug.c					\
	egg-unique.h					\
	egg-unique.c					\
	egg-precision.h					\
	egg-precision.c					\
	egg-array-float.c				\
	egg-array-float.h				\
	egg-idletime.h					\
	egg-idletime.c					\
	egg-dbus-proxy.h				\
	egg-dbus-proxy.c				\
	egg-dbus-monitor.h				\
	egg-dbus-monitor.c				\
	egg-discrete.h					\
	egg-discrete.c					\
	egg-string.h					\
	egg-string.c					\
	egg-console-kit.h				\
	egg-console-kit.c				\
	gpm-common.h					\
	gpm-common.c					\
	gpm-marshal.h					\
	gpm-marshal.c					\
	gpm-upower.c					\
	gpm-upower.h					\
	$(NULL)

mate_power_backlight_helper_SOURCES =			\
	gpm-backlight-helper.c				\
	$(NULL)

mate_power_backlight_helper_LDADD =			\
	libgpmshared.a					\
	$(GLIB_LIBS)					\
	-lm

mate_power_backlight_helper_CFLAGS =			\
	$(WARNINGFLAGS)					\
	$(NULL)

mate_power_statistics_SOURCES =			\
	gpm-statistics.c				\
	gpm-point-obj.c					\
	gpm-point-obj.h					\
	gpm-graph-widget.h				\
	gpm-graph-widget.c				\
	$(NULL)

mate_power_statistics_LDADD =				\
	libgpmshared.a					\
	$(GLIB_LIBS)					\
	$(X11_LIBS)					\
	$(UPOWER_LIBS)					\
	$(MATE_LIBS)					\
	$(UNIQUE_LIBS)					\
	$(DBUS_LIBS)					\
	-lm

mate_power_statistics_CFLAGS =				\
	$(WARNINGFLAGS)					\
	$(NULL)

mate_power_preferences_SOURCES =			\
	gpm-prefs.c					\
	gpm-prefs-core.h				\
	gpm-prefs-core.c				\
	$(NULL)

mate_power_preferences_LDADD =				\
	libgpmshared.a					\
	$(GLIB_LIBS)					\
	$(X11_LIBS)					\
	$(MATE_LIBS)					\
	$(DBUS_LIBS)					\
	$(UNIQUE_LIBS)					\
	$(GPM_EXTRA_LIBS)				\
	$(UPOWER_LIBS)					\
	-lm

mate_power_preferences_CFLAGS =			\
	$(WARNINGFLAGS)					\
	$(NULL)

mate_power_manager_SOURCES =				\
	gpm-dpms.h					\
	gpm-dpms.c					\
	gpm-phone.h					\
	gpm-phone.c					\
	gpm-backlight.h					\
	gpm-backlight.c					\
	gpm-prefs-server.h				\
	gpm-prefs-server.c				\
	gpm-idle.h					\
	gpm-idle.c					\
	gpm-load.h					\
	gpm-load.c					\
	gpm-disks.h					\
	gpm-disks.c					\
	gpm-control.h					\
	gpm-control.c					\
	gpm-button.h					\
	gpm-button.c					\
	gpm-brightness.h				\
	gpm-brightness.c				\
	gpm-main.c					\
	gpm-manager.h					\
	gpm-manager.c					\
	gpm-tray-icon.h					\
	gpm-tray-icon.c					\
	gpm-screensaver.h				\
	gpm-screensaver.c				\
	gpm-session.h					\
	gpm-session.c					\
	gpm-networkmanager.h				\
	gpm-networkmanager.c				\
	gpm-stock-icons.h				\
	gsd-media-keys-window.h				\
	gsd-media-keys-window.c				\
	gpm-engine.h					\
	gpm-engine.c					\
	$(NULL)

mate_power_manager_LDADD =				\
	libgpmshared.a					\
	$(GLIB_LIBS)					\
	$(X11_LIBS)				\
	$(GSTREAMER_LIBS)				\
	$(MATE_LIBS)					\
	$(DBUS_LIBS)					\
	$(X11_LIBS)						\
	$(CANBERRA_LIBS)				\
	$(LIBMATENOTIFY_LIBS)				\
	$(GPM_EXTRA_LIBS)				\
	$(UPOWER_LIBS)					\
	-lm

mate_power_manager_CFLAGS =				\
	$(WARNINGFLAGS)					\
	$(NULL)

mate_power_self_test_SOURCES =				\
	gpm-self-test.c					\
	egg-color.h					\
	egg-color.c					\
	egg-test.h					\
	egg-test.c					\
	egg-debug.h					\
	egg-debug.c					\
	egg-dbus-monitor.h				\
	egg-dbus-monitor.c				\
	egg-dbus-proxy.h				\
	egg-dbus-proxy.c				\
	egg-precision.h					\
	egg-precision.c					\
	egg-idletime.h					\
	egg-idletime.c					\
	egg-discrete.h					\
	egg-discrete.c					\
	egg-array-float.h				\
	egg-array-float.c				\
	egg-console-kit.h				\
	egg-console-kit.c				\
	gpm-prefs-server.h				\
	gpm-prefs-server.c				\
	gpm-control.h					\
	gpm-control.c					\
	gpm-networkmanager.h				\
	gpm-networkmanager.c				\
	gpm-dpms.h					\
	gpm-dpms.c					\
	gpm-button.h					\
	gpm-button.c					\
	gpm-screensaver.h				\
	gpm-screensaver.c				\
	gpm-engine.h					\
	gpm-engine.c					\
	gpm-phone.h					\
	gpm-phone.c					\
	gpm-idle.h					\
	gpm-idle.c					\
	gpm-session.h					\
	gpm-session.c					\
	gpm-load.h					\
	gpm-load.c					\
	gpm-marshal.h					\
	gpm-marshal.c					\
	gpm-common.h					\
	gpm-common.c					\
	gpm-upower.h					\
	gpm-upower.c					\
	$(NULL)

mate_power_self_test_LDADD =				\
	$(GLIB_LIBS)					\
	$(X11_LIBS)					\
	$(MATE_LIBS)					\
	$(GSTREAMER_LIBS)				\
	$(UPOWER_LIBS)					\
	$(DBUS_LIBS)					\
	$(X11_LIBS)						\
	$(LIBMATENOTIFY_LIBS)				\
	$(GPM_EXTRA_LIBS)				\
	-lm

mate_power_self_test_CFLAGS = -DEGG_TEST $(AM_CFLAGS) $(WARNINGFLAGS)

BUILT_SOURCES = 					\
	org.mate.PowerManager.h			\
	org.mate.PowerManager.Backlight.h		\
	gpm-marshal.c					\
	gpm-marshal.h					\
	$(NULL)

gpm-marshal.c: gpm-marshal.list
	echo "#include \"gpm-marshal.h\"" > $@ && \
	@GLIB_GENMARSHAL@ $< --prefix=gpm_marshal --body >> $@

gpm-marshal.h: gpm-marshal.list
	@GLIB_GENMARSHAL@ $< --prefix=gpm_marshal --header > $@

org.mate.PowerManager.h: org.mate.PowerManager.xml
	libtool --mode=execute dbus-binding-tool	\
		--prefix=gpm_manager			\
		--mode=glib-server			\
		--output=org.mate.PowerManager.h	\
		$(srcdir)/org.mate.PowerManager.xml

org.mate.PowerManager.Backlight.h: org.mate.PowerManager.Backlight.xml
	libtool --mode=execute dbus-binding-tool	\
		--prefix=gpm_backlight			\
		--mode=glib-server			\
		--output=org.mate.PowerManager.Backlight.h	\
		$(srcdir)/org.mate.PowerManager.Backlight.xml

clean-local:
	rm -f *~
	rm -f gpm-marshal.c gpm-marshal.h

CLEANFILES = $(BUILT_SOURCES)

TESTS = mate-power-self-test

MAINTAINERCLEANFILES =					\
	*~			      			\
	Makefile.in					\
	$(NULL)

