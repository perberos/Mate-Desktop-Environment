## Process this file with automake to produce Makefile.in

SUBDIRS = pixmaps docs data

ui_files = stickynotes-applet-menu.xml
builder_files = stickynotes.ui
applet_in_files = org.mate.applets.StickyNotesApplet.mate-panel-applet.in
service_in_files = org.mate.panel.applet.StickyNotesAppletFactory.service.in
schemas_in_files = stickynotes.schemas.in

if BUILD_STICKYNOTES_APPLET
INCLUDES =					\
	-I.					\
	-I$(srcdir)				\
	$(MATE_APPLETS3_CFLAGS)		\
	$(LIBWNCK_CFLAGS)			\
	$(XML2_CFLAGS)				\
	-DSTICKYNOTES_MENU_UI_DIR=\""$(uidir)"\"\
	-DSTICKYNOTES_ICONDIR=\"$(datadir)/pixmaps/stickynotes/\"

libexec_PROGRAMS = stickynotes_applet

stickynotes_applet_SOURCES = \
	util.h \
	util.c \
	stickynotes.h \
	stickynotes_callbacks.h \
	stickynotes_applet.h \
	stickynotes_applet_callbacks.h \
	stickynotes.c \
	stickynotes_callbacks.c \
	stickynotes_applet.c \
	stickynotes_applet_callbacks.c

stickynotes_applet_LDADD =	\
	$(MATE_APPLETS3_LIBS)	\
	$(LIBWNCK_LIBS)		\
	$(XML2_LIBS)

builder_DATA = $(builder_files)

uidir = $(datadir)/mate-2.0/ui
ui_DATA = $(ui_files)

schemasdir = @MATECONF_SCHEMA_FILE_DIR@
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)

@INTLTOOL_SCHEMAS_RULE@

appletdir   = $(datadir)/mate-panel/applets
applet_DATA = $(applet_in_files:.mate-panel-applet.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

%.mate-panel-applet: %.mate-panel-applet.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

servicedir   = $(datadir)/dbus-1/services
service_DATA = $(service_in_files:.service.in=.service)

org.mate.panel.applet.StickyNotesAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            $< > $@

CLEANFILES = $(applet_DATA) $(applet_DATA).in $(service_DATA) $(schemas_DATA)

if MATECONF_SCHEMAS_INSTALL
install-data-local:
	MATECONF_CONFIG_SOURCE=$(MATECONF_SCHEMA_CONFIG_SOURCE) \
	$(MATECONFTOOL) --makefile-install-rule $(schemas_DATA)
endif
endif

EXTRA_DIST = \
	     $(builder_files) \
	     $(ui_files) \
	     $(schemas_in_files) \
             $(service_in_files) \
	     org.mate.applets.StickyNotesApplet.mate-panel-applet.in.in


-include $(top_srcdir)/git.mk
