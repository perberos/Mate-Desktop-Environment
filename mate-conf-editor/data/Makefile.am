Utilitiesdir = $(datadir)/applications
schemasdir   = $(MATECONF_SCHEMA_FILE_DIR)

SUBDIRS=icons

Utilities_in_files = mateconf-editor.desktop.in 
Utilities_DATA = $(Utilities_in_files:.desktop.in=.desktop)
schemas_in_files = mateconf-editor.schemas.in
schemas_DATA   = $(schemas_in_files:.schemas.in=.schemas)
@INTLTOOL_DESKTOP_RULE@
@INTLTOOL_SCHEMAS_RULE@

CLEANFILES = $(Utilities_DATA) $(schemas_DATA)

EXTRA_DIST = \
	$(Utilities_in_files) \
	$(schemas_in_files)

if MATECONF_SCHEMAS_INSTALL
install-data-local:
	if test -z "$(DESTDIR)" ; then \
		for p in $(schemas_DATA) ; do \
			MATECONF_CONFIG_SOURCE=$(MATECONF_SCHEMA_CONFIG_SOURCE) $(MATECONFTOOL) --makefile-install-rule $$p ; \
		done \
	fi
else
install-data-local:
endif

