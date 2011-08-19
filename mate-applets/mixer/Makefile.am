SUBDIRS = docs

INCLUDES = -I. -I$(srcdir) 			\
	-DMIXER_MENU_UI_DIR=\""$(uidir)"\"	\
	$(MIXER_CFLAGS)				\
	$(MATE_APPLETS3_CFLAGS)		\
	$(MATEDESKTOP_CFLAGS)

libexec_PROGRAMS = mixer_applet2

mixer_applet2_SOURCES = \
	applet.c \
	dock.c \
	load.c \
	preferences.c

noinst_HEADERS = \
	applet.h \
	dock.h \
	keys.h \
	preferences.h

mixer_applet2_LDADD = 					\
	$(MIXER_LIBS) 					\
	$(MATE_APPLETS3_LIBS)				\
	$(MATEDESKTOP_LIBS)

appletdir       = $(datadir)/mate-panel/applets
applet_in_files = org.mate.applets.MixerApplet.mate-panel-applet.in
applet_DATA     = $(applet_in_files:.mate-panel-applet.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

%.mate-panel-applet: %.mate-panel-applet.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

servicedir       = $(datadir)/dbus-1/services
service_in_files = org.mate.panel.applet.MixerAppletFactory.service.in
service_DATA     = $(service_in_files:.service.in=.service)

org.mate.panel.applet.MixerAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            $< > $@

uidir   = $(datadir)/mate-2.0/ui
ui_DATA = mixer-applet-menu.xml

schemasdir   = @MATECONF_SCHEMA_FILE_DIR@
schemas_in_files = mixer.schemas.in
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)

@INTLTOOL_SCHEMAS_RULE@

EXTRA_DIST = 							\
	org.mate.applets.MixerApplet.mate-panel-applet.in.in	\
	$(service_in_files)					\
	$(ui_DATA) 						\
	$(schemas_in_files)

CLEANFILES = $(applet_DATA) $(applet_DATA).in $(service_DATA) $(schemas_DATA)

if MATECONF_SCHEMAS_INSTALL
install-data-local:
	MATECONF_CONFIG_SOURCE=$(MATECONF_SCHEMA_CONFIG_SOURCE) $(MATECONFTOOL) --makefile-install-rule $(schemas_DATA) ;
endif

-include $(top_srcdir)/git.mk
