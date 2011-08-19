NULL =

man_MANS = mate-dictionary.1

gdictappdir = $(datadir)/applications
gdictapp_in_files = mate-dictionary.desktop.in
gdictapp_DATA = $(gdictapp_in_files:.desktop.in=.desktop)

$(gdictapp_in_files): $(gdictapp_in_files:.desktop.in=.desktop.in.in)
	@sed -e "s|\@VERSION\@|@VERSION@|" $< > $@

@INTLTOOL_DESKTOP_RULE@

# dictionary sources definitions
dictsourcedir = $(datadir)/gdict-1.0/sources
dictsource_in_files = \
	default.desktop.in \
	spanish.desktop.in \
	thai.desktop.in \
	$(NULL)
dictsource_DATA = $(dictsource_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

builderdir        = $(datadir)/mate-dictionary
builder_DATA      = mate-dictionary-preferences.ui \
		    mate-dictionary-source.ui

uidir           = $(datadir)/mate-dictionary
ui_DATA         = mate-dictionary-ui.xml

if BUILD_GDICT_APPLET

# applet menu definition
appletuidir	= $(datadir)/mate-2.0/ui
appletui_DATA	= MATE_DictionaryApplet.xml

serverdir       = $(libdir)/matecomponent/servers
server_in_files = MATE_DictionaryApplet.server.in
server_DATA     = $(server_in_files:.server.in=.server)

$(server_in_files): $(server_in_files:.server.in=.server.in.in)
	@sed 						\
		-e "s|\@VERSION\@|@VERSION@|" 		\
		-e "s|\@LIBEXECDIR\@|$(libexecdir)|" 	\
	$< > $@


@INTLTOOL_SERVER_RULE@

endif # BUILD_GDICT_APPLET

schemadir   = $(MATECONF_SCHEMA_FILE_DIR)
schemas_in_file = mate-dictionary.schemas.in
schema_DATA = $(schemas_in_file:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

EXTRA_DIST = \
	MATE_DictionaryApplet.server.in.in 	\
	mate-dictionary.desktop.in.in		\
	$(dictsource_in_files)			\
	$(man_MANS) 				\
	$(schemas_in_file) 			\
	$(builder_DATA) 				\
	$(ui_DATA)				\
	$(appletui_DATA)			\
	$(NULL)

CLEANFILES = 				\
	$(dictsource_DATA)		\
	$(server_in_files) 		\
	$(server_DATA) 			\
	$(gdictapp_in_files)		\
	$(gdictapp_DATA) 		\
	$(schema_DATA)			\
	$(NULL)

if MATECONF_SCHEMAS_INSTALL
install-data-local:
	if test -z "$(DESTDIR)" ; then \
		MATECONF_CONFIG_SOURCE=$(MATECONF_SCHEMA_CONFIG_SOURCE) $(MATECONFTOOL) --makefile-install-rule $(schema_DATA) ; \
	fi
else
install-data-local:
endif
