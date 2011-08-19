SUBDIRS = icons

# This is used in MATECC_CAPPLETS_CFLAGS
cappletname = about-me

ui_files = mate-about-me-dialog.ui mate-about-me-password.ui mate-about-me-fingerprint.ui
Desktop_in_files = mate-about-me.desktop.in

mate_about_me_SOURCES =	 	\
	eel-alert-dialog.c		\
	eel-alert-dialog.h		\
	eel-gtk-macros.h		\
	mate-about-me-password.c	\
	mate-about-me-password.h	\
	e-image-chooser.c		\
	e-image-chooser.h		\
	mate-about-me-fingerprint.c	\
	mate-about-me-fingerprint.h	\
	$(MARSHALFILES)			\
	fingerprint-strings.h		\
	mate-about-me.c

MARSHALFILES = marshal.c marshal.h
BUILT_SOURCES = $(MARSHALFILES)

marshal.h: fprintd-marshal.list
	@GLIB_GENMARSHAL@ --prefix=fprintd_marshal $< --header > $@
marshal.c: fprintd-marshal.list
	@GLIB_GENMARSHAL@ --prefix=fprintd_marshal $< --body --header > $@

if BUILD_ABOUTME
bin_PROGRAMS = mate-about-me

mate_about_me_LDADD = $(MATECC_CAPPLETS_LIBS) $(LIBEBOOK_LIBS)
mate_about_me_LDFLAGS = -export-dynamic

@INTLTOOL_DESKTOP_RULE@

desktopdir = $(datadir)/applications
desktop_DATA = $(Desktop_in_files:.desktop.in=.desktop)

uidir   = $(pkgdatadir)/ui
ui_DATA = $(ui_files)

INCLUDES = \
	$(MATECC_CAPPLETS_CFLAGS) \
	$(LIBEBOOK_CFLAGS) \
	-DDATADIR="\"$(datadir)\"" \
	-DMATECC_DATA_DIR="\"$(pkgdatadir)\"" \
	-DMATECC_UI_DIR="\"$(uidir)\"" \
	-DMATECC_PIXMAP_DIR="\"$(pkgdatadir)/pixmaps\"" \
	-DMATELOCALEDIR="\"$(datadir)/locale\""

endif # BUILD_ABOUTME

CLEANFILES = $(MATECC_CAPPLETS_CLEANFILES) $(desktop_DATA) $(MARSHALFILES)
EXTRA_DIST = $(ui_files) fprintd-marshal.list

-include $(top_srcdir)/git.mk
