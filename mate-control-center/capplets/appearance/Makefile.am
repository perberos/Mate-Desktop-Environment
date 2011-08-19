SUBDIRS = data

# This is used in MATECC_CAPPLETS_CFLAGS
cappletname = appearance

bin_PROGRAMS = mate-appearance-properties

mate_appearance_properties_SOURCES = \
	appearance.h \
	appearance-desktop.c \
	appearance-desktop.h \
	appearance-font.c \
	appearance-font.h \
	appearance-main.c \
	appearance-themes.c \
	appearance-themes.h \
	appearance-style.c \
	appearance-style.h \
	mate-wp-info.c \
	mate-wp-info.h \
	mate-wp-item.c \
	mate-wp-item.h \
	mate-wp-xml.c \
	mate-wp-xml.h \
	theme-installer.c \
	theme-installer.h \
	theme-save.c \
	theme-save.h \
	theme-util.c \
	theme-util.h

AM_CFLAGS = -DMATE_DESKTOP_USE_UNSTABLE_API

mate_appearance_properties_LDADD = \
	$(top_builddir)/libwindow-settings/libmate-window-settings.la \
	$(top_builddir)/capplets/common/libcommon.la \
	$(MATECC_CAPPLETS_LIBS) \
	$(FONT_CAPPLET_LIBS) \
	$(MARCO_LIBS)
mate_appearance_properties_LDFLAGS = -export-dynamic

gtkbuilderdir = $(pkgdatadir)/ui
pixmapdir = $(pkgdatadir)/pixmaps
wallpaperdir = $(datadir)/mate-background-properties

INCLUDES = \
	$(MARCO_CFLAGS) \
	$(MATECC_CAPPLETS_CFLAGS) \
	$(FONT_CAPPLET_CFLAGS) \
	-DMATELOCALEDIR="\"$(datadir)/locale\"" \
	-DMATECC_DATA_DIR="\"$(pkgdatadir)\"" \
	-DMATECC_GTKBUILDER_DIR="\"$(gtkbuilderdir)\"" \
	-DMATECC_PIXMAP_DIR="\"$(pixmapdir)\"" \
	-DWALLPAPER_DATADIR="\"$(wallpaperdir)\""

CLEANFILES = $(MATECC_CAPPLETS_CLEANFILES)

-include $(top_srcdir)/git.mk
