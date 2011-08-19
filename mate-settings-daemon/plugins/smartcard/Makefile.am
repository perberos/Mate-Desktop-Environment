plugin_LTLIBRARIES = \
	libsmartcard.la

libsmartcard_la_SOURCES = \
	gsd-smartcard-plugin.h  \
	gsd-smartcard-plugin.c  \
	gsd-smartcard.h         \
	gsd-smartcard.c         \
	gsd-smartcard-manager.h \
	gsd-smartcard-manager.c

libsmartcard_la_CPPFLAGS = \
	-I$(top_srcdir)/mate-settings-daemon \
	-DMATE_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\" \
	-DSYSCONFDIR=\""$(sysconfdir)"\" \
	-DLIBDIR=\""$(libdir)"\" \
	-DGSD_SMARTCARD_MANAGER_NSS_DB=\""$(NSS_DATABASE)"\" \
	$(AM_CPPFLAGS)

libsmartcard_la_CFLAGS = \
	$(SETTINGS_PLUGIN_CFLAGS) \
	$(MATE_CFLAGS)	\
	$(NSS_CFLAGS)	\
	$(AM_CFLAGS)

libsmartcard_la_LDFLAGS = \
	$(GSD_PLUGIN_LDFLAGS)

libsmartcard_la_LIBADD = \
	$(SETTINGS_PLUGIN_LIBS)	\
	$(NSS_LIBS)

@GSD_INTLTOOL_PLUGIN_RULE@

plugin_in_files = \
	smartcard.mate-settings-plugin.in

plugin_DATA = $(plugin_in_files:.mate-settings-plugin.in=.mate-settings-plugin)

EXTRA_DIST = \
	$(plugin_in_files)

CLEANFILES = \
	$(plugin_DATA)

DISTCLEANFILES = \
	$(plugin_DATA)
