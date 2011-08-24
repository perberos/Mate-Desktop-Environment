NULL =

INCLUDES = \
	-DDATADIR=\""$(datadir)"\"				\
	-DMATE_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\"	\
	-DLIBEXECDIR=\""$(libexecdir)"\" 			\
	-DMATE_SETTINGS_PLUGINDIR=\""$(plugindir)"\"		\
	$(WARN_CFLAGS)						\
	$(DISABLE_DEPRECATED_CFLAGS)				\
	$(SETTINGS_DAEMON_CFLAGS)				\
	$(NULL)

noinst_LTLIBRARIES = 			\
	libgsd-profile.la		\
	$(NULL)

libgsd_profile_la_SOURCES =		\
	mate-settings-profile.c	\
	mate-settings-profile.h	\
	$(NULL)

libgsd_profile_la_CPPFLAGS = 		\
	$(DISABLE_DEPRECATED_CFLAGS)	\
	$(NULL)

libgsd_profile_la_CFLAGS =		\
	$(SIMPLE_GREETER_CFLAGS)	\
	$(NULL)

libgsd_profile_la_LIBADD =		\
	$(NULL)

libgsd_profile_la_LDFLAGS = 	\
	-export-dynamic 	\
	$(NULL)

gsddir = $(libexecdir)

gsd_PROGRAMS = \
	mate-settings-daemon

mate-settings-manager-glue.h: mate-settings-manager.xml Makefile.am
	dbus-binding-tool --prefix=mate_settings_manager --mode=glib-server $< > $@

mate-settings-client.h: mate-settings-manager.xml Makefile.am
	dbus-binding-tool --prefix=mate_settings_manager --mode=glib-client $< > $@

BUILT_SOURCES = 			\
	mate-settings-manager-glue.h	\
	mate-settings-client.h		\
	$(NULL)

apidir   = $(includedir)/mate-settings-daemon
api_DATA = 				\
	mate-settings-client.h		\
	mate-settings-plugin.h

mate_settings_daemon_SOURCES =		\
	main.c				\
	mate-settings-manager.c	\
	mate-settings-manager.h	\
	mate-settings-plugin.c		\
	mate-settings-plugin.h		\
	mate-settings-plugin-info.c	\
	mate-settings-plugin-info.h	\
	mate-settings-module.c		\
	mate-settings-module.h		\
	$(NULL)

mate_settings_daemon_CPPFLAGS = \
	$(AM_CPPFLAGS)

mate_settings_daemon_CFLAGS = 		\
	$(AM_CFLAGS)

mate_settings_daemon_LDFLAGS = 	\
	$(AM_LDFLAGS)

mate_settings_daemon_LDADD = 		\
	libgsd-profile.la		\
	$(SETTINGS_DAEMON_LIBS)

EXTRA_DIST = 				\
	$(Dbusapi_DATA)			\
	mate-settings-manager.xml	\
	$(NULL)

CLEANFILES = 				\
	$(BUILT_SOURCES)		\
	$(NULL)


# vim: ts=8
