WM_MODULE_DIR=$(libdir)/window-manager-settings

INCLUDES = 								\
	-DMATELOCALEDIR="\"$(datadir)/locale\""			\
	-DMATE_ICONDIR=\""$(datadir)/pixmaps"\"			\
	-DG_LOG_DOMAIN=\"capplet-common\"				\
	-DMATE_WM_PROPERTY_PATH=\"$(datadir)/mate/wm-properties\"	\
	-DMATE_WINDOW_MANAGER_MODULE_PATH=\""$(WM_MODULE_DIR)"\"	\
	-I$(top_srcdir)/						\
	@CAPPLET_CFLAGS@						\
	-DMARCO_THEME_DIR=\""$(datadir)/themes"\"

lib_LTLIBRARIES = libmate-window-settings.la

libmate_window_settings_la_LDFLAGS = 	\
	-export_dynamic			\
	-no-undefined			\
        -version-info 1:0:0

libmate_window_settings_la_LIBADD = $(MATECC_LIBS)

libmate_window_settings_la_SOURCES = \
	mate-window-manager.c	\
	mate-window-manager.h	\
	mate-wm-manager.c	\
	mate-wm-manager.h

libmate_window_settingsincludedir = $(includedir)/mate-window-settings-2.0

libmate_window_settingsinclude_HEADERS =	\
	mate-window-manager.h			\
	mate-wm-manager.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = mate-window-settings-2.0.pc

EXTRA_DIST = 					\
	mate-window-settings-2.0.pc.in

wms_flags = -export_dynamic -avoid-version -module
wmsdir = $(WM_MODULE_DIR)

wms_LTLIBRARIES =			\
	libmarco.la

libmarco_la_SOURCES = 		\
	marco-window-manager.c	\
	marco-window-manager.h

libmarco_la_LDFLAGS = $(wms_flags)
libmarco_la_LIBADD = 		\
	$(MATECC_LIBS)			\
	libmate-window-settings.la

-include $(top_srcdir)/git.mk
