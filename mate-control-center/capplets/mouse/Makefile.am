# This is used in MATECC_CAPPLETS_CFLAGS
cappletname = mouse

bin_PROGRAMS = mate-mouse-properties

mate_mouse_properties_LDADD = $(MATECC_CAPPLETS_LIBS)
mate_mouse_properties_SOURCES =	\
	mate-mouse-properties.c	\
	mate-mouse-accessibility.c	\
	mate-mouse-accessibility.h

@INTLTOOL_DESKTOP_RULE@

pixmapdir  = $(pkgdatadir)/pixmaps
pixmap_DATA = \
	double-click-on.png		\
	double-click-off.png		\
	double-click-maybe.png

uidir   = $(pkgdatadir)/ui
ui_DATA = mate-mouse-properties.ui

desktopdir = $(datadir)/applications
Desktop_in_files = mate-settings-mouse.desktop.in
desktop_DATA = $(Desktop_in_files:.desktop.in=.desktop)

INCLUDES = \
	$(MATECC_CAPPLETS_CFLAGS) \
	-DMATELOCALEDIR="\"$(datadir)/locale\"" \
	-DMATECC_DATA_DIR="\"$(pkgdatadir)\"" \
	-DMATECC_UI_DIR="\"$(uidir)\""
CLEANFILES = $(MATECC_CAPPLETS_CLEANFILES) $(Desktop_in_files) $(desktop_DATA)
EXTRA_DIST = $(ui_DATA) $(pixmap_DATA)

-include $(top_srcdir)/git.mk
