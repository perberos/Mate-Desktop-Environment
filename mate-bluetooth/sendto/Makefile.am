AM_CFLAGS =                                     \
        -DDATADIR=\"$(datadir)\"                \
        -DICONDIR=\"$(icondir)\"                \
	-DLOCALEDIR="\"$(datadir)/locale\""	\
	-I$(top_srcdir)/lib                     \
	-I$(top_builddir)/lib                   \
	$(SENDTO_CFLAGS)                        \
        $(NST_CFLAGS)                           \
	$(BLUETOOTH_CFLAGS)			\
	$(DBUS_CFLAGS)				\
	$(DISABLE_DEPRECATED)			\
	$(WARN_CFLAGS)

bin_PROGRAMS = bluetooth-sendto

bluetooth_sendto_SOURCES = main.c

bluetooth_sendto_LDADD = $(top_builddir)/lib/libcommon.la $(top_builddir)/lib/libmate-bluetooth.la $(SENDTO_LIBS)

man_MANS = bluetooth-sendto.1

if HAVE_NST
plugindir = $(libdir)/caja-sendto/plugins

plugin_LTLIBRARIES = libnstbluetooth.la

libnstbluetooth_la_SOURCES = caja-sendto-plugin.c
libnstbluetooth_la_LDFLAGS = -module -avoid-version
libnstbluetooth_la_LIBADD = $(BLUETOOTH_LIBS) ../lib/libmate-bluetooth.la

convertdir=$(datadir)/MateConf/gsettings
convert_DATA=mate-bluetooth-nst

gsettingsschema_in_files = org.mate.Bluetooth.nst.gschema.xml.in
gsettings_SCHEMAS = $(gsettingsschema_in_files:.xml.in=.xml)
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

endif

EXTRA_DIST = $(man_MANS) mate-bluetooth-nst org.mate.Bluetooth.nst.gschema.xml.in caja-sendto-plugin.c

MAINTAINERCLEANFILES = Makefile.in
