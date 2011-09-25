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
	-DMDMCONFDIR=\"$(mdmconfdir)\"			\
	-DMDMLOCALEDIR=\"$(mdmlocaledir)\"		\
	-DLIBDIR=\"$(libdir)\"				\
	-DLIBEXECDIR=\"$(libexecdir)\"			\
	-DLOGDIR=\"$(logdir)\"				\
	-DPIXMAPDIR=\"$(pixmapdir)\"			\
	-DSBINDIR=\"$(sbindir)\"			\
	-DMATELOCALEDIR=\""$(datadir)/locale"\"	\
	-DMDM_DEFAULTS_CONF=\"$(MDM_DEFAULTS_CONF)\"	\
	-DMDM_CUSTOM_CONF=\"$(MDM_CUSTOM_CONF)\"	\
	-DMDM_OLD_CONF=\"$(MDM_OLD_CONF)\"		\
	$(COMMON_CFLAGS)				\
	$(NULL)

noinst_LTLIBRARIES = 		\
	libmdmcommon.la		\
	$(NULL)

BUILT_SOURCES =			\
	mdm-settings-glue.h	\
	mdm-marshal.c		\
	mdm-marshal.h		\
	$(NULL)

mdmdir = $(datadir)/mdm
mdm_DATA = gdb-cmd

libexec_PROGRAMS = mdm-crash-logger
mdm_crash_logger_SOURCES = mdm-crash-logger.c
mdm_crash_logger_CPPFLAGS = \
	$(AM_CPPFLAGS)				\
	$(GTHREAD_CFLAGS)			\
	-DG_DISABLE_DEPRECATED			\
	-DBINDIR=\"$(bindir)\"			\
	-DSBINDIR=\"$(sbindir)\"		\
	-DDATADIR=\"$(datadir)\"		\
	-DSYSCONFDIR=\"$(sysconfdir)\"		\
	-DLOCALSTATEDIR=\"$(localstatedir)\"	\
	$(NULL)
mdm_crash_logger_LDADD = $(GTHREAD_LIBS)

mdm-settings-glue.h: mdm-settings.xml Makefile.am
	dbus-binding-tool --prefix=mdm_settings --mode=glib-server --output=mdm-settings-glue.h $(srcdir)/mdm-settings.xml

mdm-marshal.c: mdm-marshal.list
	echo "#include \"mdm-marshal.h\"" > $@ && \
	@GLIB_GENMARSHAL@ $< --prefix=mdm_marshal --body >> $@

mdm-marshal.h: mdm-marshal.list
	@GLIB_GENMARSHAL@ $< --prefix=mdm_marshal --header > $@

if MKDTEMP_MISSING
MKDTEMP_FILES = mkdtemp.c mkdtemp.h
else
MKDTEMP_FILES =
endif

libmdmcommon_la_SOURCES =	\
	mdm-address.h		\
	mdm-address.c		\
	mdm-common.h		\
	mdm-common.c		\
	mdm-common-unknown-origin.h	\
	mdm-common-unknown-origin.c	\
	mdm-profile.c		\
	mdm-profile.h		\
	mdm-settings.c 		\
	mdm-settings.h		\
	mdm-settings-backend.c 	\
	mdm-settings-backend.h	\
	mdm-settings-desktop-backend.c 	\
	mdm-settings-desktop-backend.h	\
	mdm-settings-keys.h	\
	mdm-settings-utils.h	\
	mdm-settings-utils.c	\
	mdm-settings-direct.c 	\
	mdm-settings-direct.h	\
	mdm-settings-client.h	\
	mdm-settings-client.c	\
	mdm-log.h		\
	mdm-log.c		\
	mdm-md5.h		\
	mdm-md5.c		\
	mdm-signal-handler.h	\
	mdm-signal-handler.c	\
	$(MKDTEMP_FILES) 	\
	$(NULL)

nodist_libmdmcommon_la_SOURCES =	\
	mdm-marshal.c		\
	mdm-marshal.h

libmdmcommon_la_CPPFLAGS = 		\
	$(AM_CPPFLAGS)			\
	$(DISABLE_DEPRECATED_CFLAGS)	\
	$(NULL)

libmdmcommon_la_CFLAGS =		\
	$(COMMON_CFLAGS)		\
	$(NULL)

libmdmcommon_la_LIBADD =		\
	$(NULL)

libmdmcommon_la_LDFLAGS = 	\
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
	libmdmcommon.la		\
	$(COMMON_LIBS)		\
	$(NULL)

test_settings_server_SOURCES = 		\
	test-settings-server.c	 	\
	$(NULL)

test_settings_server_LDADD =		\
	libmdmcommon.la			\
	$(COMMON_LIBS)			\
	$(NULL)

test_settings_client_SOURCES = 		\
	test-settings-client.c	 	\
	$(NULL)

test_settings_client_LDADD =		\
	libmdmcommon.la			\
	$(COMMON_LIBS)			\
	$(NULL)

CLEANFILES =			\
	$(BUILT_SOURCES)	\
	$(NULL)

EXTRA_DIST = 			\
	mdm-settings.xml	\
	mdm-marshal.list	\
	mkdtemp.c 		\
	mkdtemp.h		\
	$(mdm_DATA)		\
	$(NULL)
