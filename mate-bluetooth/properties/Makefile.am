
bin_PROGRAMS = bluetooth-properties

bluetooth_properties_SOURCES = main.c general.h general.c adapter.h adapter.c

bluetooth_properties_LDADD = $(top_builddir)/lib/libmate-bluetooth.la $(top_builddir)/lib/libcommon.la $(PROPS_LIBS)

AM_CFLAGS = $(PROPS_CFLAGS) $(WARN_CFLAGS) $(DISABLE_DEPRECATED) -DPKGDATADIR="\"$(pkgdatadir)\""

INCLUDES = -I$(top_srcdir)/lib

man_MANS = bluetooth-properties.1

desktopdir = $(datadir)/applications
desktop_in_in_files = bluetooth-properties.desktop.in.in
desktop_in_files = bluetooth-properties.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

ui_DATA = properties-no-adapter.ui properties-adapter-off.ui properties-killed-adapter.ui
uidir = $(pkgdatadir)

convertdir=$(datadir)/MateConf/gsettings
convert_DATA=mate-bluetooth

gsettingsschema_in_files = org.mate.Bluetooth.gschema.xml.in
gsettings_SCHEMAS = $(gsettingsschema_in_files:.xml.in=.xml)
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

CLEANFILES = $(desktop_DATA) $(schema_DATA)

EXTRA_DIST = $(man_MANS) $(desktop_in_in_files) $(schema_in_files) $(ui_DATA) $(convert_DATA) $(gsettingsschema_in_files)

MAINTAINERCLEANFILES = Makefile.in


