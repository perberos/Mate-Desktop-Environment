# This is used in MATECC_CAPPLETS_CFLAGS
cappletname = keyboard

bin_PROGRAMS = mate-keyboard-properties

mate_keyboard_properties_SOURCES = \
	mate-keyboard-properties.c \
	mate-keyboard-properties-a11y.c \
	mate-keyboard-properties-a11y.h \
	mate-keyboard-properties-xkb.c \
	mate-keyboard-properties-xkbmc.c \
	mate-keyboard-properties-xkblt.c \
	mate-keyboard-properties-xkbltadd.c \
	mate-keyboard-properties-xkbot.c \
	mate-keyboard-properties-xkbpv.c \
	mate-keyboard-properties-xkb.h

mate_keyboard_properties_LDADD = $(MATECC_CAPPLETS_LIBS) $(LIBMATEKBDUI_LIBS)

@INTLTOOL_DESKTOP_RULE@

uidir   = $(pkgdatadir)/ui
dist_ui_DATA = mate-keyboard-properties-a11y-notifications.ui \
               mate-keyboard-properties-dialog.ui \
               mate-keyboard-properties-layout-chooser.ui \
               mate-keyboard-properties-model-chooser.ui \
               mate-keyboard-properties-options-dialog.ui

desktopdir = $(datadir)/applications
Desktop_in_files = keyboard.desktop.in
desktop_DATA = $(Desktop_in_files:.desktop.in=.desktop)

INCLUDES = \
	$(MATECC_CAPPLETS_CFLAGS) \
	$(LIBMATEKBDUI_CFLAGS) \
	-DMATELOCALEDIR="\"$(datadir)/locale\"" \
	-DMATECC_DATA_DIR="\"$(pkgdatadir)\"" \
	-DMATECC_UI_DIR="\"$(uidir)\""
CLEANFILES = $(MATECC_CAPPLETS_CLEANFILES) $(Desktop_in_files) $(desktop_DATA)
EXTRA_DIST = $(ui_DATA)

-include $(top_srcdir)/git.mk
