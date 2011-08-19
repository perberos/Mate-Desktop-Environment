SUBDIRS = help

INCLUDES =					\
	-I.					\
	-I$(srcdir)				\
	-DCHARPICK_MENU_UI_DIR=\""$(uidir)"\"	\
	$(MATE_APPLETS3_CFLAGS)		\
	$(GUCHARMAP_CFLAGS)

libexec_PROGRAMS = charpick_applet2

charpick_applet2_SOURCES = charpick.c \
                          charpick.h \
                          properties.c

charpick_applet2_LDADD = \
		       $(MATE_APPLETS3_LIBS)	\
		       $(GUCHARMAP_LIBS)

appletdir       = $(datadir)/mate-panel/applets
applet_in_files = org.mate.applets.CharpickerApplet.mate-panel-applet.in
applet_DATA     = $(applet_in_files:.mate-panel-applet.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

%.mate-panel-applet: %.mate-panel-applet.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

servicedir       = $(datadir)/dbus-1/services
service_in_files = org.mate.panel.applet.CharpickerAppletFactory.service.in
service_DATA     = $(service_in_files:.service.in=.service)

org.mate.panel.applet.CharpickerAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            $< > $@

CLEANFILES = $(applet_DATA) $(applet_DATA).in $(service_DATA) $(schemas_DATA)

schemasdir   = @MATECONF_SCHEMA_FILE_DIR@
schemas_in_files = charpick.schemas.in
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)

@INTLTOOL_SCHEMAS_RULE@
uidir = $(datadir)/mate-2.0/ui
ui_DATA = charpick-applet-menu.xml

EXTRA_DIST =							\
	org.mate.applets.CharpickerApplet.mate-panel-applet.in.in	\
	$(service_in_files)					\
	$(ui_DATA)						\
	$(schemas_in_files)

if MATECONF_SCHEMAS_INSTALL
install-data-local:
	MATECONF_CONFIG_SOURCE=$(MATECONF_SCHEMA_CONFIG_SOURCE) $(MATECONFTOOL) --makefile-install-rule $(schemas_DATA) ;
endif

-include $(top_srcdir)/git.mk
