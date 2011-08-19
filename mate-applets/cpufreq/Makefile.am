SUBDIRS = help pixmaps src

schemasdir       = @MATECONF_SCHEMA_FILE_DIR@
schemas_in_files = cpufreq-applet.schemas.in
schemas_DATA     = $(schemas_in_files:.schemas.in=.schemas)

@INTLTOOL_SCHEMAS_RULE@

appletdir       = $(datadir)/mate-panel/applets
applet_in_files = org.mate.applets.CPUFreqApplet.mate-panel-applet.in
applet_DATA     = $(applet_in_files:.mate-panel-applet.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

%.mate-panel-applet: %.mate-panel-applet.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

servicedir       = $(datadir)/dbus-1/services
service_in_files = org.mate.panel.applet.CPUFreqAppletFactory.service.in
service_DATA     = $(service_in_files:.service.in=.service)

org.mate.panel.applet.CPUFreqAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            $< > $@

builder_DATA = cpufreq-preferences.ui

uidir = $(datadir)/mate-2.0/ui
ui_DATA = cpufreq-applet-menu.xml

if MATECONF_SCHEMAS_INSTALL
install-data-local:
	-MATECONF_CONFIG_SOURCE=$(MATECONF_SCHEMA_CONFIG_SOURCE) $(MATECONFTOOL) --makefile-install-rule $(top_builddir)/cpufreq/$(schemas_DATA) ;
endif

CLEANFILES = \
             $(applet_DATA)			\
             $(applet_DATA).in			\
             $(service_DATA)			\
	     cpufreq-applet.schemas             \
	     *.bak                              \
	     *.gladep

EXTRA_DIST = \
             org.mate.applets.CPUFreqApplet.mate-panel-applet.in.in	\
             $(service_in_files)				\
	     $(schemas_in_files)             			\
	     $(builder_DATA)                 			\
	     $(ui_DATA)
	     
					


-include $(top_srcdir)/git.mk
