schemadir   = $(MATECONF_SCHEMA_FILE_DIR)
schema_in_files = \
	system_http_proxy.schemas.in \
	system_smb.schemas.in \
	system_dns_sd.schemas.in \
	desktop_default_applications.schemas.in \
	desktop_mate_url_handlers.schemas.in

schema_DATA = $(schema_in_files:.schemas.in=.schemas)

@INTLTOOL_SCHEMAS_RULE@

# don't do this if we are building in eg. rpm
if MATECONF_SCHEMAS_INSTALL
if OS_WIN32
# Work around MSYS feature. If we pass MATECONF_CONFIG_SOURCE as an
# environment variable, MSYS is "helpful" and thinks the colons in the
# variable means that is is a search path, and changes the colons into
# semicolons and slashes into backslashes. Usually this feature is
# useful, but not this time.
install-data-local:
	if test -z "$(DESTDIR)" ; then \
		for p in $(schema_DATA) ; do \
			(echo set MATECONF_CONFIG_SOURCE=$(MATECONF_SCHEMA_CONFIG_SOURCE); \
			 echo $(MATECONFTOOL) --makefile-install-rule $$p) >_temp.bat; \
			cmd /c _temp.bat; \
		done \
	fi
else
install-data-local:
	if test -z "$(DESTDIR)" ; then \
		for p in $(schema_DATA) ; do \
			MATECONF_CONFIG_SOURCE=$(MATECONF_SCHEMA_CONFIG_SOURCE) $(MATECONFTOOL) --makefile-install-rule $$p; \
		done \
	fi
endif
else
install-data-local:
endif

EXTRA_DIST = $(schema_in_files) $(schema_DATA)

DISTCLEANFILES = $(schema_DATA)
