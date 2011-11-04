# This is used in MATECC_CAPPLETS_CFLAGS
cappletname = display

uidir = $(pkgdatadir)/ui
dist_ui_DATA = display-capplet.ui

bin_PROGRAMS = mate-display-properties

sbin_PROGRAMS = mate-display-properties-install-systemwide

mate_display_properties_SOURCES =	\
	xrandr-capplet.c		\
	scrollarea.c			\
	foo-marshal.c			\
	scrollarea.h			\
	foo-marshal.h

mate_display_properties_LDFLAGS = -export-dynamic
mate_display_properties_LDADD = \
	$(top_builddir)/capplets/common/libcommon.la \
	$(DISPLAY_CAPPLET_LIBS)

mate_display_properties_install_systemwide_SOURCES =	\
	mate-display-properties-install-systemwide.c

mate_display_properties_install_systemwide_LDADD =	\
	$(GLIB_LIBS)

polkit_policydir = $(datadir)/polkit-1/actions
dist_polkit_policy_DATA =					\
	org.mate.randr.policy

# You will need a recent intltool or the patch from this bug
# http://bugzilla.gnome.org/show_bug.cgi?id=462312
@INTLTOOL_POLICY_RULE@

@INTLTOOL_DESKTOP_RULE@

icons16dir = $(datadir)/icons/hicolor/16x16/apps
dist_icons16_DATA = icons/16x16/mate-preferences-desktop-display.png
icons22dir = $(datadir)/icons/hicolor/22x22/apps
dist_icons22_DATA = icons/22x22/mate-preferences-desktop-display.png
icons24dir = $(datadir)/icons/hicolor/24x24/apps
dist_icons24_DATA = icons/24x24/mate-preferences-desktop-display.png
icons32dir = $(datadir)/icons/hicolor/32x32/apps
dist_icons32_DATA = icons/32x32/mate-preferences-desktop-display.png
iconssvgdir = $(datadir)/icons/hicolor/scalable/apps
dist_iconssvg_DATA = icons/scalable/mate-preferences-desktop-display.svg

desktopdir = $(datadir)/applications
Desktop_in_files = display-properties.desktop.in
desktop_DATA = $(Desktop_in_files:.desktop.in=.desktop)

INCLUDES   = $(DISPLAY_CAPPLET_CFLAGS) \
             $(MATECC_CAPPLETS_CFLAGS) \
	     -DSBINDIR="\"$(sbindir)\"" \
	     -DUIDIR="\"$(uidir)\"" \
	     -DMATELOCALEDIR="\"$(datadir)/locale\"" \
	     -DMATECC_DATA_DIR="\"$(pkgdatadir)\""

CLEANFILES = $(MATECC_CAPPLETS_CLEANFILES) $(Desktop_in_files) $(desktop_DATA)

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor
install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

EXTRA_DIST = org.mate.randr.policy.in
DISTCLEANFILES = org.mate.randr.policy

-include $(top_srcdir)/git.mk
