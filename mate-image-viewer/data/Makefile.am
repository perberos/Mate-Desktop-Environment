SUBDIRS = pixmaps icons

@INTLTOOL_DESKTOP_RULE@

DESKTOP_IN_FILES= eog.desktop.in.in
DESKTOP_FILES= $(DESKTOP_IN_FILES:.desktop.in.in=.desktop)

desktopdir = $(datadir)/applications
desktop_DATA = $(DESKTOP_FILES)

@INTLTOOL_SCHEMAS_RULE@

schemas_in_files = eog.schemas.in
schemasdir = $(MATECONF_SCHEMA_FILE_DIR)
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)

gladedir = $(pkgdatadir)
glade_DATA = eog-image-properties-dialog.ui \
	     eog-multiple-save-as-dialog.ui \
	     eog-preferences-dialog.ui

uidir = $(datadir)/eog
UI_FILES = \
	eog-ui.xml \
	eog-toolbar.xml

ui_DATA = $(UI_FILES)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = eog.pc

gtkrcdir = $(pkgdatadir)
gtkrc_DATA = gtkrc

install-data-local:
if MATECONF_SCHEMAS_INSTALL
	if test -z "$(DESTDIR)" ; then \
	for p in $(schemas_DATA) ; do \
	MATECONF_CONFIG_SOURCE=$(MATECONF_SCHEMA_CONFIG_SOURCE) $(MATECONFTOOL) --makefile-install-rule $(top_builddir)/data/$$p >&1 > /dev/null; \
	done \
	fi
endif

EXTRA_DIST =			\
        $(gtkrc_DATA)		\
        $(glade_DATA)		\
        $(DESKTOP_IN_FILES)	\
        $(schemas_in_files)	\
	$(UI_FILES)

DISTCLEANFILES = 		\
        $(DESKTOP_FILES)        \
        $(schemas_DATA)

-include $(top_srcdir)/git.mk
