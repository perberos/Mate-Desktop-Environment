EXTRA_DIST =

INCLUDES = \
	-DMATECC_DATA_DIR="\"$(pkgdatadir)\""				\
	-DMATELOCALEDIR="\"$(datadir)/locale\""			\
	-DGTK_ENGINE_DIR="\"$(GTK_ENGINE_DIR)\"" 			\
	-DG_LOG_DOMAIN=\"capplet-common\"				\
	-DINSTALL_PREFIX=\"$(prefix)\"					\
	-I$(top_srcdir)							\
	-I$(top_srcdir)/libwindow-settings				\
	-DPIXMAP_DIR=\""$(datadir)/mate-control-center/pixmaps"\"	\
	$(CAPPLET_CFLAGS)						\
	$(DBUS_CFLAGS)							\
	$(MATE_DESKTOP_CFLAGS)						\
	$(MARCO_CFLAGS)						\
	$(GSD_DBUS_CFLAGS)						\
	$(GIO_CFLAGS)


noinst_LTLIBRARIES = libcommon.la

libcommon_la_SOURCES = \
	activate-settings-daemon.c	\
	activate-settings-daemon.h	\
	capplet-stock-icons.c		\
	capplet-stock-icons.h		\
	capplet-util.c			\
	capplet-util.h			\
	file-transfer-dialog.c		\
	file-transfer-dialog.h		\
	mateconf-property-editor.c		\
	mateconf-property-editor.h		\
	mateconf-property-editor-marshal.c	\
	mateconf-property-editor-marshal.h \
	mate-theme-apply.c		\
	mate-theme-apply.h 		\
	mate-theme-info.c		\
	mate-theme-info.h		\
	gtkrc-utils.c			\
	gtkrc-utils.h			\
	theme-thumbnail.c		\
	theme-thumbnail.h		\
	wm-common.c			\
	wm-common.h

libcommon_la_LIBADD =							\
	$(top_builddir)/libwindow-settings/libmate-window-settings.la  \
	$(MARCO_LIBS)						\
	$(DBUS_LIBS)							\
	$(MATE_DESKTOP_LIBS)						\
	$(GIO_LIBS)

mate_theme_test_SOURCES = \
	mate-theme-test.c

mate_theme_test_LDADD = 						\
	libcommon.la							\
	$(MATECC_CAPPLETS_LIBS)

noinst_PROGRAMS = \
	mate-theme-test

-include $(top_srcdir)/git.mk
