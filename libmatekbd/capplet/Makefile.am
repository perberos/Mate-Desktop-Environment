bin_PROGRAMS = matekbd-indicator-plugins-capplet

uidir="$(pkgdatadir)/ui"
ui_DATA = matekbd-indicator-plugins.ui \
	matekbd-indicator-plugins_add.ui

@INTLTOOL_DESKTOP_RULE@

desktopdir = $(datadir)/applications
Desktop_in_files = matekbd-indicator-plugins-capplet.desktop.in
desktop_DATA = $(Desktop_in_files:.desktop.in=.desktop)

matekbd_indicator_plugins_capplet_CFLAGS = \
	-I$(top_srcdir)	-Wall 				\
	$(MATECONF_CFLAGS)					\
	$(GTK_CFLAGS)				\
	$(LIBXKLAVIER_CFLAGS)				\
	-I$(top_srcdir)/intl				\
	-DSYS_PLUGIN_DIR=\"$(libdir)/matekbd/\"	\
	-DG_LOG_DOMAIN=\"MateKbdIndicatorPluginsCapplet\"	\
        -DPREFIX=\"$(prefix)\"				\
        -DUIDIR=\"$(uidir)\"                      \
        -DDATADIR=\"$(datadir)\"                        \
        -DSYSCONFDIR=\"$(sysconfdir)\"                  \
        -DMATELOCALEDIR="\"$(datadir)/locale\""	\
        -DLIBDIR=\"$(libdir)\"

matekbd_indicator_plugins_capplet_SOURCES = matekbd-indicator-plugins-capplet.c \
                                    matekbd-indicator-plugins-add.c \
                                    matekbd-indicator-plugins-capplet.h

matekbd_indicator_plugins_capplet_LDFLAGS = -export-dynamic

matekbd_indicator_plugins_capplet_LDADD = \
            	$(MATECONF_LIBS) 	\
            	$(GTK_LIBS) 	\
            	$(LIBXKLAVIER_LIBS)	\
		$(top_builddir)/libmatekbd/libmatekbd.la \
		$(top_builddir)/libmatekbd/libmatekbdui.la

EXTRA_DIST = matekbd-indicator-plugins-capplet.desktop.in.in \
             $(ui_DATA) \
             $(pixmap_DATA)

DISTCLEANFILES = $(desktop_DATA)

