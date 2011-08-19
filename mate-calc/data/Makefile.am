uidir = $(datadir)/gcalctool
ui_DATA = \
	preferences.ui \
	buttons-basic.ui \
	buttons-advanced.ui \
	buttons-financial.ui \
	buttons-programming.ui

gsettings_SCHEMAS = org.mate.gcalctool.gschema.xml
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

Utilitiesdir = $(datadir)/applications
Utilities_in_files = gcalctool.desktop.in
Utilities_DATA = $(Utilities_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

man1_MANS = gcalctool.1

EXTRA_DIST = $(ui_DATA) org.mate.gcalctool.gschema.xml.in gcalctool.desktop.in $(man1_MANS)

DISTCLEANFILES = \
	Makefile.in \
	gcalctool.desktop \
	$(gsettings_SCHEMAS)
