# This is used in MATECC_CAPPLETS_CFLAGS
cappletname = default-applications

bin_PROGRAMS = mate-default-applications-properties

bin_SCRIPTS = mate-at-visual mate-at-mobility

mate_default_applications_properties_LDADD = $(MATECC_CAPPLETS_LIBS)
mate_default_applications_properties_SOURCES = \
	mate-da-capplet.c mate-da-capplet.h \
	mate-da-xml.c mate-da-xml.h \
	mate-da-item.c mate-da-item.h

@INTLTOOL_DESKTOP_RULE@

uidir = $(pkgdatadir)/ui
dist_ui_DATA = mate-default-applications-properties.ui

mate-at-visual: mate-at-commandline.in
	cp $< $@

mate-at-mobility: mate-at-commandline.in
	cp $< $@

desktopdir = $(datadir)/applications
Desktop_in_files = default-applications.desktop.in
desktop_DATA = $(Desktop_in_files:.desktop.in=.desktop)

pkgconfigdir = $(datadir)/pkgconfig
pkgconfig_DATA = mate-default-applications.pc

autostartdir = $(sysconfdir)/xdg/autostart
autostart_in_files = mate-at-session.desktop.in
autostart_DATA = $(autostart_in_files:.desktop.in=.desktop)

xmldata_in_files = mate-default-applications.xml.in
xmldatadir = $(pkgdatadir)/default-apps
xmldata_DATA = $(xmldata_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@

INCLUDES = \
	$(MATECC_CAPPLETS_CFLAGS) \
	$(DEFAULT_APPLICATIONS_CAPPLET_CFLAGS) \
	-DMATELOCALEDIR=\""$(datadir)/locale"\"\
	-DMATECC_UI_DIR=\""$(uidir)"\" \
	-DMATECC_APPS_DIR=\""$(xmldatadir)"\"

icons16dir = $(datadir)/icons/mate/16x16/apps
dist_icons16_DATA = icons/16x16/preferences-desktop-default-applications.png
icons22dir = $(datadir)/icons/mate/22x22/apps
dist_icons22_DATA = icons/22x22/preferences-desktop-default-applications.png
icons24dir = $(datadir)/icons/mate/24x24/apps
dist_icons24_DATA = icons/24x24/preferences-desktop-default-applications.png
icons32dir = $(datadir)/icons/mate/32x32/apps
dist_icons32_DATA = icons/32x32/preferences-desktop-default-applications.png
icons48dir = $(datadir)/icons/mate/48x48/apps
dist_icons48_DATA = icons/48x48/preferences-desktop-default-applications.png

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor
uninstall-hook: update-icon-cache
install-data-hook: update-icon-cache
update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi


CLEANFILES = $(MATECC_CAPPLETS_CLEANFILES) $(Desktop_in_files) $(desktop_DATA) $(xmldata_DATA) $(autostart_DATA) $(bin_SCRIPTS)
EXTRA_DIST = $(xmldata_in_files) mate-default-applications.pc.in

-include $(top_srcdir)/git.mk
