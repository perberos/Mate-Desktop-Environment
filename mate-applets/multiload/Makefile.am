SUBDIRS = docs

INCLUDES =			\
	-I$(srcdir)		\
	-DMULTILOAD_MENU_UI_DIR=\""$(uidir)"\" \
	$(MATE_APPLETS3_CFLAGS)	\
	$(GTOP_APPLETS_CFLAGS)	\
	$(MATEDESKTOP_CFLAGS)

libexec_PROGRAMS = multiload-applet-2

multiload_applet_2_SOURCES = \
			global.h	\
			linux-proc.h	\
			load-graph.h	\
			linux-proc.c	\
			load-graph.c	\
			main.c	\
			properties.c	\
			netspeed.c netspeed.h \
			autoscaler.c	\
			autoscaler.h

multiload_applet_2_LDADD = \
				$(MATE_APPLETS3_LIBS) 	\
				$(GTOP_APPLETS_LIBS)	\
				$(MATEDESKTOP_LIBS)

schemasdir   = @MATECONF_SCHEMA_FILE_DIR@
schemas_in_files = multiload.schemas.in
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)

@INTLTOOL_SCHEMAS_RULE@

EXTRA_DIST =							\
	org.mate.applets.MultiLoadApplet.mate-panel-applet.in.in	\
	$(service_in_files)					\
	$(schemas_in_files)					\
	$(ui_DATA)

uidir		= $(datadir)/mate-2.0/ui
ui_DATA		= multiload-applet-menu.xml

appletdir       = $(datadir)/mate-panel/applets
applet_in_files = org.mate.applets.MultiLoadApplet.mate-panel-applet.in
applet_DATA     = $(applet_in_files:.mate-panel-applet.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

%.mate-panel-applet: %.mate-panel-applet.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

servicedir       = $(datadir)/dbus-1/services
service_in_files = org.mate.panel.applet.MultiLoadAppletFactory.service.in
service_DATA     = $(service_in_files:.service.in=.service)

org.mate.panel.applet.MultiLoadAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            $< > $@

CLEANFILES = $(applet_DATA) $(applet_DATA).in $(service_DATA) $(schemas_DATA)

if MATECONF_SCHEMAS_INSTALL
install-data-local:
	MATECONF_CONFIG_SOURCE=$(MATECONF_SCHEMA_CONFIG_SOURCE) $(MATECONFTOOL) --makefile-install-rule $(schemas_DATA) ;
endif

-include $(top_srcdir)/git.mk
