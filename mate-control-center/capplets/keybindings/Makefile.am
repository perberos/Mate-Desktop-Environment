# This is used in MATECC_CAPPLETS_CFLAGS
cappletname = keybinding

bin_PROGRAMS = mate-keybinding-properties

mate_keybinding_properties_LDADD = $(MATECC_CAPPLETS_LIBS)
mate_keybinding_properties_SOURCES = 	\
	mate-keybinding-properties.c	\
	eggcellrendererkeys.c		\
	eggcellrendererkeys.h		\
	eggaccelerators.c		\
	eggaccelerators.h

@INTLTOOL_DESKTOP_RULE@

uidir   = $(pkgdatadir)/ui
ui_DATA = mate-keybinding-properties.ui

desktopdir = $(datadir)/applications
Desktop_in_files = mate-keybinding.desktop.in
desktop_DATA = $(Desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_XML_NOMERGE_RULE@

xmldir       = $(pkgdatadir)/keybindings
xml_in_files = 00-multimedia-key.xml.in 01-desktop-key.xml.in
xml_DATA     = $(xml_in_files:.xml.in=.xml)

pkgconfigdir = $(datadir)/pkgconfig
pkgconfig_DATA = mate-keybindings.pc

INCLUDES = \
	$(MATECC_CAPPLETS_CFLAGS) \
	-DMATELOCALEDIR="\"$(datadir)/locale\"" \
	-DMATECC_DATA_DIR="\"$(pkgdatadir)\"" \
	-DMATECC_UI_DIR="\"$(uidir)\"" \
	-DMATECC_KEYBINDINGS_DIR="\"$(pkgdatadir)/keybindings\""
CLEANFILES = \
	$(MATECC_CAPPLETS_CLEANFILES) \
	$(Desktop_in_files) \
	$(desktop_DATA) \
	$(xml_DATA)
EXTRA_DIST = $(ui_DATA) $(xml_in_files) mate-keybindings.pc.in


-include $(top_srcdir)/git.mk
