glade_files = 				\
	amarok.glade			\
	dbus.glade			\
	mpd.glade			\
	password.glade			\
	plugins.glade			\
	preferences.glade		\
	xmms1.glade			\
	xmms2.glade

pkgdata_DATA =				\
	music-applet.png		\
	music-applet-set-star.png	\
	music-applet-unset-star.png	\
	$(glade_files)

appicon32_DATA =			\
	music-applet-mpd.png
appicon32dir = $(datadir)/icons/hicolor/32x32/apps

appiconscale_DATA =			\
	music-applet-xmms2.svg
appiconscaledir = $(datadir)/icons/hicolor/scalable/apps

serverdir       = $(libdir)/matecomponent/servers
server_in_files = MATE_Music_Applet.server.in
server_DATA     = $(server_in_files:.server.in=.server)

$(server_in_files): $(server_in_files:.server.in=.server.in.in)
	sed -e "s|\@LIBEXECDIR\@|$(libexecdir)|" $< | sed -e "s|\@PKGDATADIR\@|$(pkgdatadir)|" > $@

@INTLTOOL_SERVER_RULE@

schemasdir       = @MATECONF_SCHEMA_FILE_DIR@
schemas_in_files = music-applet.schemas.in
schemas_DATA     = $(schemas_in_files:.schemas.in=.schemas)

@INTLTOOL_SCHEMAS_RULE@

uidir   = $(libdir)/mate-2.0/ui
ui_DATA = MATE_Music_Applet.xml

EXTRA_DIST = $(pkgdata_DATA) 					\
	     $(appicon32_DATA)					\
	     $(appiconscale_DATA)				\
	     $(server_in_files:.server.in=.server.in.in) 	\
	     $(schemas_in_files) 				\
	     $(ui_DATA)

CLEANFILES = $(server_in_files) $(server_DATA) $(schemas_DATA)			\
	     $(glade_files:.glade=.gladep)					\
	     $(glade_files:.glade=.glade.bak)					\
	     $(glade_files:.glade=.gladep.bak)

if MATECONF_SCHEMAS_INSTALL
install-data-local:
	MATECONF_CONFIG_SOURCE=$(MATECONF_SCHEMA_CONFIG_SOURCE) $(MATECONFTOOL) --makefile-install-rule $(schemas_DATA) ;
endif
