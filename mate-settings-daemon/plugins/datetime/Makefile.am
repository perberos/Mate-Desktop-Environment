dbus_servicesdir = $(datadir)/dbus-1/system-services
dbus_confdir = $(sysconfdir)/dbus-1/system.d
polkitdir = $(datadir)/polkit-1/actions

dbus_services_in_files = org.mate.SettingsDaemon.DateTimeMechanism.service.in
polkit_in_files = org.mate.settingsdaemon.datetimemechanism.policy.in

gsd-datetime-mechanism-glue.h: $(srcdir)/gsd-datetime-mechanism.xml
	$(AM_V_GEN)dbus-binding-tool \
			--prefix=gsd_datetime_mechanism --mode=glib-server	\
			--output=gsd-datetime-mechanism-glue.h			\
			$(srcdir)/gsd-datetime-mechanism.xml

if HAVE_POLKIT
libexec_PROGRAMS = gsd-datetime-mechanism
endif

gsd_datetime_mechanism_SOURCES =		\
	gsd-datetime-mechanism.c		\
	gsd-datetime-mechanism.h		\
	gsd-datetime-mechanism-main.c	\
	system-timezone.c			\
	system-timezone.h


if HAVE_POLKIT
BUILT_SOURCES = gsd-datetime-mechanism-glue.h
endif

AM_CFLAGS = $(SETTINGS_PLUGIN_CFLAGS) $(POLKIT_CFLAGS)
gsd_datetime_mechanism_LDADD = $(POLKIT_LIBS) $(SETTINGS_PLUGIN_LIBS)


if HAVE_POLKIT
dbus_services_DATA = $(dbus_services_in_files:.service.in=.service)

$(dbus_services_DATA): $(dbus_services_in_files)
	$(AM_V_GEN)sed -e "s|\@LIBEXECDIR\@|$(libexecdir)|" $< > $@

dbus_conf_DATA = org.mate.SettingsDaemon.DateTimeMechanism.conf

@INTLTOOL_POLICY_RULE@
polkit_DATA = $(polkit_in_files:.policy.in=.policy)

else
dbus_services_DATA =
dbus_conf_DATA =
polkit_DATA =
endif

EXTRA_DIST =						\
	$(dbus_services_in_files)			\
	org.mate.SettingsDaemon.DateTimeMechanism.conf	\
	$(polkit_in_files)				\
	gsd-datetime-mechanism.xml

CLEANFILES = 		\
	org.mate.SettingsDaemon.DateTimeMechanism.service	\
	org.mate.settingsdaemon.datetimemechanism.policy	\
	$(BUILT_SOURCES)