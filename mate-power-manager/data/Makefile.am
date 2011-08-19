SUBDIRS = icons

@INTLTOOL_SCHEMAS_RULE@
schemadir = $(sysconfdir)/mateconf/schemas
schema_in_files = mate-power-manager.schemas.in
schema_DATA = $(schema_in_files:.schemas.in=.schemas)

@INTLTOOL_DESKTOP_RULE@
desktopdir = $(datadir)/applications
desktop_in_files = mate-power-preferences.desktop.in \
		   mate-power-statistics.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@
autostartdir = $(sysconfdir)/xdg/autostart
autostart_in_files = mate-power-manager.desktop.in
autostart_DATA = $(autostart_in_files:.desktop.in=.desktop)

servicedir       = $(DBUS_SERVICES_DIR)
service_in_files = mate-power-manager.service.in
service_DATA     = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	@sed -e "s|\@servicedir\@|$(bindir)|" $< > $@

pkgdata_DATA =						\
	gpm-statistics.ui				\
	gpm-feedback-widget.ui				\
	gpm-prefs.ui					\
	acme.ui

EXTRA_DIST =						\
	mate-power-manager.schemas.in			\
	$(schema_in_files)				\
	$(desktop_in_files)				\
	$(autostart_in_files)				\
	$(service_in_files)				\
	$(pkgdata_DATA)

if MATECONF_SCHEMAS_INSTALL
install-data-local:
	-MATECONF_CONFIG_SOURCE=$(MATECONF_SCHEMA_CONFIG_SOURCE) \
	$(MATECONFTOOL) --makefile-install-rule $(schema_DATA)
endif

if MATECONF_SCHEMAS_INSTALL
uninstall-local:
	-MATECONF_CONFIG_SOURCE=$(MATECONF_SCHEMA_CONFIG_SOURCE) \
	$(MATECONFTOOL) --makefile-uninstall-rule $(schema_DATA)
endif

clean-local :
	rm -f *~

DISTCLEANFILES =					\
	mate-power-manager.desktop			\
	mate-power-preferences.desktop			\
	mate-power-statistics.desktop			\
	mate-power-manager.schemas			\
	mate-power-manager.service
