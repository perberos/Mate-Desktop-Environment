lib_LTLIBRARIES = libmatekbd.la libmatekbdui.la

uidir="$(pkgdatadir)/ui"
ui_DATA = show-layout.ui

common_CFLAGS = \
	-I$(top_srcdir) -Wall 			\
	$(GDK_CFLAGS)					\
	$(MATECONF_CFLAGS)					\
	$(LIBXKLAVIER_CFLAGS)				\
	$(XLIB_CFLAGS)					\
	-I$(top_srcdir)/intl				\
	-DSYS_PLUGIN_DIR=\"$(libdir)/matekbd/\"	\
	-DG_LOG_DOMAIN=\"MateKbdIndicator\"		\
	-DDATADIR=\"$(datadir)\"			\
	-DSYSCONFDIR=\"$(sysconfdir)\"			\
	-DLIBDIR=\"$(libdir)\"				

libmatekbd_la_CFLAGS = $(common_CFLAGS)

libmatekbdui_la_CFLAGS = $(common_CFLAGS)		\
	-DUIDIR=\"$(uidir)\"				\
	$(GTK_CFLAGS)

common_LDFLAGS = -version-info @VERSION_INFO@ -no-undefined

common_LIBADD = \
		$(GDK_LIBS) \
		$(MATECONF_LIBS) \
		$(LIBXKLAVIER_LIBS) \
		$(XLIB_LIBS)

libmatekbd_la_LDFLAGS = $(common_LDFLAGS)
libmatekbd_la_LIBADD =  $(common_LIBADD)

libmatekbdui_la_LDFLAGS = $(common_LDFLAGS)
libmatekbdui_la_LIBADD = libmatekbd.la \
                           $(common_LIBADD) \
                           $(GTK_LIBS)

libmatekbd_la_SOURCES = \
                         gkbd-desktop-config.c \
                         gkbd-keyboard-config.c \
                         gkbd-util.c

libmatekbdui_la_SOURCES = \
                         gkbd-indicator-config.c \
                         gkbd-indicator.c \
                         gkbd-status.c \
                         gkbd-indicator-marshal.c \
                         gkbd-indicator-plugin-manager.c \
                         gkbd-keyboard-drawing-marshal.c \
                         gkbd-keyboard-drawing.c

BUILT_SOURCES = gkbd-indicator-marshal.c \
	gkbd-indicator-marshal.h         \
        gkbd-keyboard-drawing-marshal.c            \
        gkbd-keyboard-drawing-marshal.h

CLEANFILES = $(BUILT_SOURCES) $(schema_DATA)

matekbdincdir = $(includedir)/libmatekbd
matekbdinc_HEADERS = \
                      gkbd-desktop-config.h \
                      gkbd-keyboard-config.h \
                      gkbd-indicator.h \
                      gkbd-status.h \
                      gkbd-indicator-config.h \
                      gkbd-indicator-plugin.h \
                      gkbd-keyboard-drawing.h \
                      gkbd-util.h

@INTLTOOL_DESKTOP_RULE@

@INTLTOOL_SCHEMAS_RULE@

noinst_HEADERS = gkbd-indicator-plugin-manager.h \
                 $(extra_nih) \
                 gkbd-config-private.h

EXTRA_DIST = gkbd-indicator-marshal.list \
             gkbd-keyboard-drawing-marshal.list \
             $(schema_in_files) $(ui_DATA)

GLIB_GENMARSHAL = $(shell pkg-config --variable=glib_genmarshal glib-2.0)

gkbd-indicator-marshal.h: gkbd-indicator-marshal.list
	$(AM_V_GEN)$(GLIB_GENMARSHAL) --prefix=gkbd_indicator $(srcdir)/gkbd-indicator-marshal.list --header > $@

gkbd-indicator-marshal.c: gkbd-indicator-marshal.h
	$(AM_V_GEN)$(GLIB_GENMARSHAL) --prefix=gkbd_indicator $(srcdir)/gkbd-indicator-marshal.list --body > $@

gkbd-keyboard-drawing-marshal.h: gkbd-keyboard-drawing-marshal.list
	$(AM_V_GEN)( $(GLIB_GENMARSHAL) --prefix=gkbd_keyboard_drawing $(srcdir)/gkbd-keyboard-drawing-marshal.list \
                            --header > gkbd-keyboard-drawing-marshal.tmp \
          && mv gkbd-keyboard-drawing-marshal.tmp gkbd-keyboard-drawing-marshal.h ) \
        || ( rm -f gkbd-keyboard-drawing-marshal.tmp && exit 1 )
 
gkbd-keyboard-drawing-marshal.c: gkbd-keyboard-drawing-marshal.h
	$(AM_V_GEN)( $(GLIB_GENMARSHAL) --prefix=gkbd_keyboard_drawing $(srcdir)/gkbd-keyboard-drawing-marshal.list \
                            --body > gkbd-keyboard-drawing-marshal.tmp \
          && mv gkbd-keyboard-drawing-marshal.tmp gkbd-keyboard-drawing-marshal.c ) \
        || ( rm -f gkbd-keyboard-drawing-marshal.tmp && exit 1 )

schemadir   = $(MATECONF_SCHEMA_FILE_DIR)
schema_in_files= desktop_mate_peripherals_keyboard_xkb.schemas.in

schema_DATA = $(schema_in_files:.schemas.in=.schemas)

if MATECONF_SCHEMAS_INSTALL
# don't do this if we are building in eg. rpm
install-data-local:
	if test -z "$(DESTDIR)" ; then \
		for p in $(schema_DATA) ; do \
			MATECONF_CONFIG_SOURCE=$(MATECONF_SCHEMA_CONFIG_SOURCE) $(MATECONFTOOL) --makefile-install-rule $$p; \
		done \
	fi
else
install-data-local:
endif

