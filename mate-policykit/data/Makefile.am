schema_in_files =  polkit-mate.schemas.in
schemadir   = $(MATECONF_SCHEMA_FILE_DIR)
schema_DATA = $(schema_in_files:.schemas.in=.schemas)

@INTLTOOL_SCHEMAS_RULE@

# don't do this if we are building in eg. rpm
if MATECONF_SCHEMAS_INSTALL
install-data-local:
	if test -z "$(DESTDIR)" ; then \
		for p in $(schema_DATA) ; do \
			MATECONF_CONFIG_SOURCE=$(MATECONF_SCHEMA_CONFIG_SOURCE) $(MATECONFTOOL) --makefile-install-rule $$p; \
		done \
	fi
endif

@INTLTOOL_DESKTOP_RULE@
desktopdir = $(datadir)/applications
desktop_in_files = polkit-mate-authorization.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

servicedir       = $(datadir)/dbus-1/services
service_in_files = org.mate.PolicyKit.service.in                                  \
		   org.mate.PolicyKit.AuthorizationManager.service.in             \
		   mate-org.freedesktop.PolicyKit.AuthenticationAgent.service.in
service_DATA     = $(service_in_files:.service.in=.service)

org.mate.PolicyKit.service : org.mate.PolicyKit.service.in Makefile
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

mate-org.freedesktop.PolicyKit.AuthenticationAgent.service : mate-org.freedesktop.PolicyKit.AuthenticationAgent.service.in Makefile
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

org.mate.PolicyKit.AuthorizationManager.service : org.mate.PolicyKit.AuthorizationManager.service.in Makefile
	@sed -e "s|\@bindir\@|$(bindir)|" $< > $@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = polkit-mate.pc

DISTCLEANFILES = $(service_DATA) $(desktop_DATA) $(pkgconfig_DATA)

EXTRA_DIST =			\
	$(service_in_files)	\
	$(desktop_in_files)	\
	$(schema_in_files)	\
	polkit-mate.pc.in

clean-local :
	rm -f *~ $(service_DATA) $(pkgconfig_DATA) $(schema_DATA)
