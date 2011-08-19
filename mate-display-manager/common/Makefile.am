## Process this file with automake to produce Makefile.in

NULL =

AM_CPPFLAGS = \
	-I.						\
	-I..						\
	-DMATELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DAUTHDIR=\"$(authdir)\"			\
	-DBINDIR=\"$(bindir)\"				\
	-DDATADIR=\"$(datadir)\"			\
	-DDMCONFDIR=\"$(dmconfdir)\"			\
	-DGDMCONFDIR=\"$(gdmconfdir)\"			\
	-DGDMLOCALEDIR=\"$(gdmlocaledir)\"		\
	-DLIBDIR=\"$(libdir)\"				\
	-DLIBEXECDIR=\"$(libexecdir)\"			\
	-DLOGDIR=\"$(logdir)\"				\
	-DPIXMAPDIR=\"$(pixmapdir)\"			\
	-DSBINDIR=\"$(sbindir)\"			\
	-DMATELOCALEDIR=\""$(datadir)/locale"\"	\
	-DGDM_DEFAULTS_CONF=\"$(GDM_DEFAULTS_CONF)\"	\
	-DGDM_CUSTOM_CONF=\"$(GDM_CUSTOM_CONF)\"	\
	-DGDM_OLD_CONF=\"$(GDM_OLD_CONF)\"		\
	$(COMMON_CFLAGS)				\
	$(NULL)

noinst_LTLIBRARIES = 		\
	libgdmcommon.la		\
	$(NULL)

BUILT_SOURCES =			\
	gdm-settings-glue.h	\
	gdm-marshal.c		\
	gdm-marshal.h		\
	$(NULL)

gdmdir = $(datadir)/gdm
gdm_DATA = gdb-cmd

libexec_PROGRAMS = gdm-crash-logger
gdm_crash_logger_SOURCES = gdm-crash-logger.c
gdm_crash_logger_CPPFLAGS = \
	$(AM_CPPFLAGS)				\
	$(GTHREAD_CFLAGS)			\
	-DG_DISABLE_DEPRECATED			\
	-DBINDIR=\"$(bindir)\"			\
	-DSBINDIR=\"$(sbindir)\"		\
	-DDATADIR=\"$(datadir)\"		\
	-DSYSCONFDIR=\"$(sysconfdir)\"		\
	-DLOCALSTATEDIR=\"$(localstatedir)\"	\
	$(NULL)
gdm_crash_logger_LDADD = $(GTHREAD_LIBS)

gdm-settings-glue.h: gdm-settings.xml Makefile.am
	dbus-binding-tool --prefix=gdm_settings --mode=glib-server --output=gdm-settings-glue.h $(srcdir)/gdm-settings.xml

gdm-marshal.c: gdm-marshal.list
	echo "#include \"gdm-marshal.h\"" > $@ && \
	@GLIB_GENMARSHAL@ $< --prefix=gdm_marshal --body >> $@

gdm-marshal.h: gdm-marshal.list
	@GLIB_GENMARSHAL@ $< --prefix=gdm_marshal --header > $@

if MKDTEMP_MISSING
MKDTEMP_FILES = mkdtemp.c mkdtemp.h
else
MKDTEMP_FILES =
endif

libgdmcommon_la_SOURCES =	\
	gdm-address.h		\
	gdm-address.c		\
	gdm-common.h		\
	gdm-common.c		\
	gdm-common-unknown-origin.h	\
	gdm-common-unknown-origin.c	\
	gdm-profile.c		\
	gdm-profile.h		\
	gdm-settings.c 		\
	gdm-settings.h		\
	gdm-settings-backend.c 	\
	gdm-settings-backend.h	\
	gdm-settings-desktop-backend.c 	\
	gdm-settings-desktop-backend.h	\
	gdm-settings-keys.h	\
	gdm-settings-utils.h	\
	gdm-settings-utils.c	\
	gdm-settings-direct.c 	\
	gdm-settings-direct.h	\
	gdm-settings-client.h	\
	gdm-settings-client.c	\
	gdm-log.h		\
	gdm-log.c		\
	gdm-md5.h		\
	gdm-md5.c		\
	gdm-signal-handler.h	\
	gdm-signal-handler.c	\
	$(MKDTEMP_FILES) 	\
	$(NULL)

nodist_libgdmcommon_la_SOURCES =	\
	gdm-marshal.c		\
	gdm-marshal.h

libgdmcommon_la_CPPFLAGS = 		\
	$(AM_CPPFLAGS)			\
	$(DISABLE_DEPRECATED_CFLAGS)	\
	$(NULL)

libgdmcommon_la_CFLAGS =		\
	$(COMMON_CFLAGS)		\
	$(NULL)

libgdmcommon_la_LIBADD =		\
	$(NULL)

libgdmcommon_la_LDFLAGS = 	\
	-export-dynamic 	\
	$(NULL)

noinst_PROGRAMS = 		\
	test-settings-server	\
	test-settings-client	\
	test-log		\
	$(NULL)

test_log_SOURCES = 		\
	test-log.c	 	\
	$(NULL)

test_log_LDADD =		\
	libgdmcommon.la		\
	$(COMMON_LIBS)		\
	$(NULL)

test_settings_server_SOURCES = 		\
	test-settings-server.c	 	\
	$(NULL)

test_settings_server_LDADD =		\
	libgdmcommon.la			\
	$(COMMON_LIBS)			\
	$(NULL)

test_settings_client_SOURCES = 		\
	test-settings-client.c	 	\
	$(NULL)

test_settings_client_LDADD =		\
	libgdmcommon.la			\
	$(COMMON_LIBS)			\
	$(NULL)

CLEANFILES =			\
	$(BUILT_SOURCES)	\
	$(NULL)

EXTRA_DIST = 			\
	gdm-settings.xml	\
	gdm-marshal.list	\
	mkdtemp.c 		\
	mkdtemp.h		\
	$(gdm_DATA)		\
	$(NULL)
