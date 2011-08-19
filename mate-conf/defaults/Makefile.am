libexec_PROGRAMS = mateconf-defaults-mechanism

mateconf-defaults-glue.h: $(srcdir)/mateconf-defaults.xml
	dbus-binding-tool --prefix=mateconf_defaults --mode=glib-server      \
			  --output=mateconf-defaults-glue.h                  \
			  $(srcdir)/mateconf-defaults.xml


mateconf_defaults_mechanism_SOURCES = \
	mateconf-defaults.h \
	mateconf-defaults.c \
	mateconf-defaults-glue.h \
	mateconf-defaults-main.c

INCLUDES = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	-DSYSMATECONFDIR=\"$(sysmateconfdir)\" \
	$(DEFAULTS_CFLAGS)

mateconf_defaults_mechanism_LDADD = \
	$(top_builddir)/mateconf/libmateconf-2.la \
	$(DEFAULTS_LIBS)

BUILT_SOURCES = mateconf-defaults-glue.h

dbus_servicesdir = $(datadir)/dbus-1/system-services
dbus_confdir = $(sysconfdir)/dbus-1/system.d
polkitdir = $(datadir)/polkit-1/actions

dbus_services_in_files = org.mate.MateConf.Defaults.service.in
polkit_in_files = org.mate.mateconf.defaults.policy.in

dbus_services_DATA = $(dbus_services_in_files:.service.in=.service)

$(dbus_services_DATA): $(dbus_services_in_files)
	sed -e "s|\@LIBEXECDIR\@|$(libexecdir)|" $< > $@

dbus_conf_DATA = org.mate.MateConf.Defaults.conf

@INTLTOOL_POLICY_RULE@
polkit_DATA = $(polkit_in_files:.policy.in=.policy)

EXTRA_DIST =					\
	$(dbus_services_in_files)		\
	org.mate.MateConf.Defaults.conf		\
	$(polkit_in_files)			\
	mateconf-defaults.xml

CLEANFILES =			\
	$(BUILT_SOURCES)	\
	$(polkit_DATA)		\
	$(dbus_services_DATA)

DISTCLEANFILES = org.mate.mateconf.defaults.policy

