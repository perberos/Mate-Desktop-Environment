Desktopdir = $(datadir)/applications
SUBDIRS = doc icons pixmaps src

@INTLTOOL_DESKTOP_RULE@
@INTLTOOL_SCHEMAS_RULE@

Desktop_in_files = mate-volume-control.desktop.in.in
Desktop_DATA = $(Desktop_in_files:.desktop.in.in=.desktop)
schemadir=$(MATECONF_SCHEMA_FILE_DIR)
schema_DATA=mate-volume-control.schemas

install-schemas:
	 MATECONF_CONFIG_SOURCE=$(MATECONF_SCHEMA_CONFIG_SOURCE) $(MATECONFTOOL) --makefile-install-rule $(schema_DATA) || true

if MATECONF_SCHEMAS_INSTALL
install-data-local: install-schemas
else
install-data-local:
endif

EXTRA_DIST = $(Desktop_in_files) \
	mate-volume-control.schemas.in 


CLEANFILES = $(Desktop_DATA) $(schema_DATA)
DISTCLEANFILES = $(Desktop_in_files:.desktop.in.in=.desktop.in)

-include $(top_srcdir)/git.mk
