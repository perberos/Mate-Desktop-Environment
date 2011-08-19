SUBDIRS = src docs

appletdir       = $(datadir)/mate-panel/applets
applet_in_files = org.mate.applets.TrashApplet.mate-panel-applet.in
applet_DATA     = $(applet_in_files:.mate-panel-applet.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

%.mate-panel-applet: %.mate-panel-applet.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

servicedir       = $(datadir)/dbus-1/services
service_in_files = org.mate.panel.applet.TrashAppletFactory.service.in
service_DATA     = $(service_in_files:.service.in=.service)

org.mate.panel.applet.TrashAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            $< > $@

uidir = $(datadir)/mate-2.0/ui
ui_DATA = trashapplet-menu.xml

builder_DATA = trashapplet-empty-progress.ui

EXTRA_DIST = \
	$(ui_DATA) \
	org.mate.applets.TrashApplet.mate-panel-applet.in.in \
	$(service_in_files) \
	$(builder_DATA)

CLEANFILES = $(applet_DATA) $(applet_DATA).in $(service_DATA)

-include $(top_srcdir)/git.mk
