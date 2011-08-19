
INCLUDES = $(FONT_VIEWER_CFLAGS) $(MATECC_CAPPLETS_CFLAGS) -DDIRECTORY_DIR=\"$(directorydir)\" \
  -DMATELOCALEDIR=\"$(datadir)/locale\"

bin_PROGRAMS = mate-thumbnail-font mate-font-viewer

mate_thumbnail_font_LDADD = $(MATECC_CAPPLETS_LIBS) $(FONT_VIEWER_LIBS)
mate_thumbnail_font_SOURCES = ftstream-vfs.c font-thumbnailer.c totem-resources.c totem-resources.h

mate_font_viewer_LDADD = $(MATECC_CAPPLETS_LIBS) $(FONT_VIEWER_LIBS)
mate_font_viewer_SOURCES = ftstream-vfs.c font-view.c

schemasdir = $(MATECONF_SCHEMA_FILE_DIR)
schemas_DATA = fontilus.schemas

desktopdir = $(datadir)/applications
desktop_in_files = mate-font-viewer.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

install-data-local: install-mateconf-schemas install-desktop-database

if MATECONF_SCHEMAS_INSTALL
install-mateconf-schemas:
	if test -z "$(DESTDIR)"; then \
	  MATECONF_CONFIG_SOURCE=$(MATECONF_SCHEMA_CONFIG_SOURCE) $(MATECONFTOOL) --makefile-install-rule fontilus.schemas; \
	fi
else
install-mateconf-schemas:
endif

install-desktop-database: install-desktopDATA
	update-desktop-database $(DESTDIR)$(desktopdir)

uninstall-local:
	rm -f $(DESTDIR)$(desktopdir)/mimeinfo.cache

@INTLTOOL_DESKTOP_RULE@
@INTLTOOL_DIRECTORY_RULE@
@INTLTOOL_SCHEMAS_RULE@

CLEANFILES = $(desktop_in_files) $(desktop_DATA) \
  $(schemas_DATA) $(directory_DATA)

EXTRA_DIST = \
  fontilus.schemas.in \
  mate-font-viewer.desktop.in.in

-include $(top_srcdir)/git.mk
