SUBDIRS = art

# ******************************************************************************
# Panel applet matecomponent stuff
# ******************************************************************************
serverdir = $(libdir)/matecomponent/servers
server_in_files = Invest_Applet.server.in
server_DATA = \
	$(server_in_files:.server.in=.server) \
	MATE_GtikApplet.server

# ******************************************************************************
# MateConf Schemas
# ******************************************************************************
# MateConf Preference Schemas
#schemadir = $(MATECONF_SCHEMA_FILE_DIR)
#schema_in_files = deskbar-applet.schemas.in
#schema_DATA = $(schema_in_files:.schemas.in=.schemas)

# ******************************************************************************
# Misc data
# ******************************************************************************
uidir = $(datadir)/mate-2.0/ui
ui_DATA = \
	Invest_Applet.xml
builder_DATA = \
	prefs-dialog.ui \
	financialchart.ui

investbindir = $(libdir)/invest-applet

# ******************************************************************************
# Build rules
# ******************************************************************************
@INTLTOOL_SERVER_RULE@
@INTLTOOL_SCHEMAS_RULE@

$(server_in_files): $(server_in_files:.server.in=.server.in.in)
	sed -e "s|\@LIBEXECDIR\@|$(libexecdir)|" $< > $@

#if MATECONF_SCHEMAS_INSTALL
#install-data-local: install-schemas
#install-schemas:
#	MATECONF_CONFIG_SOURCE=$(MATECONF_SCHEMA_CONFIG_SOURCE) \
#		$(MATECONFTOOL) --makefile-install-rule $(schema_DATA)
#endif

DISTCLEANFILES = \
	$(server_in_files) \
	$(server_in_files:.server.in=.server)

EXTRA_DIST = \
	Invest_Applet.server.in.in \
	MATE_GtikApplet.server \
	$(ui_DATA) \
	$(builder_DATA)

-include $(top_srcdir)/git.mk
