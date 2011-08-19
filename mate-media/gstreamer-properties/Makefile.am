SUBDIRS = . help
bin_PROGRAMS = gstreamer-properties

gstreamer_properties_LDADD = $(GSTPROPS_LIBS)
gstreamer_properties_SOURCES = \
	gstreamer-properties.c gstreamer-properties-structs.h \
	pipeline-tests.h pipeline-tests.c \
	pipeline-constants.c

@INTLTOOL_DESKTOP_RULE@

iconsdir   = $(datadir)/gstreamer-properties/icons
icons_DATA = gstreamer-properties.png

desktop_iconsdir = $(datadir)/icons/hicolor/48x48/apps
desktop_icons_DATA = gstreamer-properties.png

uidir   = $(datadir)/gstreamer-properties/
ui_DATA = gstreamer-properties.ui

desktopdir = $(datadir)/applications
Desktop_in_files = gstreamer-properties.desktop.in.in
desktop_DATA = $(Desktop_in_files:.desktop.in.in=.desktop)

AM_CPPFLAGS = \
	$(GSTPROPS_CFLAGS) \
	-DGSTPROPS_UIDIR=\""$(uidir)"\" \
	-DGSTPROPS_ICONDIR=\""$(iconsdir)"\" \
	-DDATADIR=\""$(datadir)"\" \
	-DMATELOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DG_LOG_DOMAIN="\"gstreamer-properties\"" \
	$(DISABLE_DEPRECATED)

CLEANFILES = $(GSTPROPS_CLEANFILES) $(desktop_DATA)
DISTCLEANFILES = $(Desktop_in_files:.desktop.in.in=.desktop.in)
EXTRA_DIST = $(icons_DATA) $(ui_DATA) $(Desktop_in_files)

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor
install-data-hook:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

-include $(top_srcdir)/git.mk
