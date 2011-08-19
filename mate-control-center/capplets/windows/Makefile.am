# This is used in MATECC_CAPPLETS_CFLAGS
cappletname = window

bin_PROGRAMS = mate-window-properties

mate_window_properties_LDADD = $(MATECC_CAPPLETS_LIBS) 	\
	$(top_builddir)/libwindow-settings/libmate-window-settings.la

mate_window_properties_SOURCES = 		\
	mate-window-properties.c

@INTLTOOL_DESKTOP_RULE@

uidir   = $(pkgdatadir)/ui
ui_DATA = mate-window-properties.ui

desktopdir = $(datadir)/applications
Desktop_in_files = window-properties.desktop.in
desktop_DATA = $(Desktop_in_files:.desktop.in=.desktop)

INCLUDES   = $(MATECC_CAPPLETS_CFLAGS)					\
	     -I$(top_srcdir)/libwindow-settings \
	     -DMATE_WINDOW_MANAGER_MODULE_PATH=\""$(libdir)/window-manager-settings"\" \
	     -DUIDIR=\""$(uidir)"\"  	\
	     -DMATELOCALEDIR="\"$(datadir)/locale\""			\
	     -DPIXMAPDIR=\""$(pixmapdir)"\"

CLEANFILES = $(MATECC_CAPPLETS_CLEANFILES) $(Desktop_in_files) $(desktop_DATA)
EXTRA_DIST = $(ui_DATA)


-include $(top_srcdir)/git.mk
