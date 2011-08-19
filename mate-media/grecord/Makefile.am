SUBDIRS = src doc icons

EXTRA_DIST = \
	README AUTHORS INSTALL ChangeLog.pre-2-26 \
	mate-sound-recorder.desktop.in.in \
	mate-sound-recorder.schemas.in


Applicationsdir = $(datadir)/applications
Applications_in_files = mate-sound-recorder.desktop.in.in
Applications_DATA = $(Applications_in_files:.desktop.in.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

schemadir=$(MATECONF_SCHEMA_FILE_DIR)
schema_DATA=mate-sound-recorder.schemas

@INTLTOOL_SCHEMAS_RULE@

CLEANFILES=$(Applications_DATA) $(schema_DATA)
DISTCLEANFILES=$(Applications_in_files:.desktop.in.in=.desktop.in)
install-schemas:
	MATECONF_CONFIG_SOURCE=$(MATECONF_SCHEMA_CONFIG_SOURCE) $(MATECONFTOOL) --makefile-install-rule $(schema_DATA) || true

if MATECONF_SCHEMAS_INSTALL
install-data-local: install-schemas
else
install-data-local:
endif

-include $(top_srcdir)/git.mk
