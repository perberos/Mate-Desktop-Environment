SUBDIRS = help

INCLUDES =					\
	-I.					\
	-I$(srcdir)				\
	-DDRIVEMOUNT_MENU_UI_DIR=\""$(uidir)"\"	\
	$(MATE_APPLETS3_CFLAGS)

libexec_PROGRAMS = drivemount_applet2

drivemount_applet2_SOURCES =	\
	drivemount.c		\
	drive-list.c		\
	drive-list.h		\
	drive-button.c		\
	drive-button.h

drivemount_applet2_LDADD =	\
	$(MATE_APPLETS3_LIBS)


appletdir       = $(datadir)/mate-panel/applets
applet_in_files = org.mate.applets.DriveMountApplet.mate-panel-applet.in
applet_DATA     = $(applet_in_files:.mate-panel-applet.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

%.mate-panel-applet: %.mate-panel-applet.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

servicedir       = $(datadir)/dbus-1/services
service_in_files = org.mate.panel.applet.DriveMountAppletFactory.service.in
service_DATA     = $(service_in_files:.service.in=.service)

org.mate.panel.applet.DriveMountAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            $< > $@

uidir = $(datadir)/mate-2.0/ui
ui_DATA = drivemount-applet-menu.xml

schemasdir   = $(MATECONF_SCHEMA_FILE_DIR)
schemas_in_files = drivemount.schemas.in
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)

@INTLTOOL_SCHEMAS_RULE@

CLEANFILES = $(applet_DATA) $(applet_DATA).in $(service_DATA) $(schemas_DATA)

EXTRA_DIST = \
 org.mate.applets.DriveMountApplet.mate-panel-applet.in.in	\
 $(service_in_files)					\
 $(schemas_in_files)					\
 $(ui_DATA)

if MATECONF_SCHEMAS_INSTALL
install-data-local:
	if test -z "$(DESTDIR)" ; then \
		for p in $(schemas_DATA) ; do \
		MATECONF_CONFIG_SOURCE=$(MATECONF_SCHEMA_CONFIG_SOURCE) $(MATECONFTOOL) --makefile-install-rule $(top_builddir)/drivemount/$$p ; \
		done ; \
	fi
else
install-data-local:
endif

-include $(top_srcdir)/git.mk
