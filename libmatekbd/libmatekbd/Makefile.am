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
                         matekbd-desktop-config.c \
                         matekbd-keyboard-config.c \
                         matekbd-util.c

libmatekbdui_la_SOURCES = \
                         matekbd-indicator-config.c \
                         matekbd-indicator.c \
                         matekbd-status.c \
                         matekbd-indicator-marshal.c \
                         matekbd-indicator-plugin-manager.c \
                         matekbd-keyboard-drawing-marshal.c \
                         matekbd-keyboard-drawing.c

BUILT_SOURCES = matekbd-indicator-marshal.c \
	matekbd-indicator-marshal.h         \
        matekbd-keyboard-drawing-marshal.c            \
        matekbd-keyboard-drawing-marshal.h

CLEANFILES = $(BUILT_SOURCES) $(schema_DATA)

matekbdincdir = $(includedir)/libmatekbd
matekbdinc_HEADERS = \
                      matekbd-desktop-config.h \
                      matekbd-keyboard-config.h \
                      matekbd-indicator.h \
                      matekbd-status.h \
                      matekbd-indicator-config.h \
                      matekbd-indicator-plugin.h \
                      matekbd-keyboard-drawing.h \
                      matekbd-util.h

@INTLTOOL_DESKTOP_RULE@

@INTLTOOL_SCHEMAS_RULE@

noinst_HEADERS = matekbd-indicator-plugin-manager.h \
                 $(extra_nih) \
                 matekbd-config-private.h

EXTRA_DIST = matekbd-indicator-marshal.list \
             matekbd-keyboard-drawing-marshal.list \
             $(schema_in_files) $(ui_DATA)

GLIB_GENMARSHAL = $(shell pkg-config --variable=glib_genmarshal glib-2.0)

matekbd-indicator-marshal.h: matekbd-indicator-marshal.list
	$(AM_V_GEN)$(GLIB_GENMARSHAL) --prefix=matekbd_indicator $(srcdir)/matekbd-indicator-marshal.list --header > $@

matekbd-indicator-marshal.c: matekbd-indicator-marshal.h
	$(AM_V_GEN)$(GLIB_GENMARSHAL) --prefix=matekbd_indicator $(srcdir)/matekbd-indicator-marshal.list --body > $@

matekbd-keyboard-drawing-marshal.h: matekbd-keyboard-drawing-marshal.list
	$(AM_V_GEN)( $(GLIB_GENMARSHAL) --prefix=matekbd_keyboard_drawing $(srcdir)/matekbd-keyboard-drawing-marshal.list \
                            --header > matekbd-keyboard-drawing-marshal.tmp \
          && mv matekbd-keyboard-drawing-marshal.tmp matekbd-keyboard-drawing-marshal.h ) \
        || ( rm -f matekbd-keyboard-drawing-marshal.tmp && exit 1 )
 
matekbd-keyboard-drawing-marshal.c: matekbd-keyboard-drawing-marshal.h
	$(AM_V_GEN)( $(GLIB_GENMARSHAL) --prefix=matekbd_keyboard_drawing $(srcdir)/matekbd-keyboard-drawing-marshal.list \
                            --body > matekbd-keyboard-drawing-marshal.tmp \
          && mv matekbd-keyboard-drawing-marshal.tmp matekbd-keyboard-drawing-marshal.c ) \
        || ( rm -f matekbd-keyboard-drawing-marshal.tmp && exit 1 )

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

