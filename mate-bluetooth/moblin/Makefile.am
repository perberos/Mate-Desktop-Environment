SUBDIRS = moblin-copy-n-paste theme moblin-icons

bin_PROGRAMS = bluetooth-panel

bluetooth_panel_SOURCES = main.c moblin-panel.c moblin-panel.h

bluetooth_panel_LDADD = $(top_builddir)/lib/libcommon.la $(top_builddir)/lib/libmate-bluetooth.la \
			$(top_builddir)/moblin/moblin-copy-n-paste/libmoblin.la \
			$(top_builddir)/wizard/libwizard.la $(MOBLIN_LIBS)

AM_CFLAGS = $(MOBLIN_CFLAGS) $(WARN_CFLAGS) $(DISABLE_DEPRECATED) -DPKGDATADIR="\"$(pkgdatadir)\""

INCLUDES = -I$(top_srcdir)/lib -I$(top_srcdir)/wizard -I$(top_srcdir)/moblin/moblin-copy-n-paste

ui_DATA = pin.ui
uidir = $(pkgdatadir)


desktop_in_files = bluetooth-panel.desktop.in.in
desktop_data = $(desktop_in_files:.desktop.in.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

autostartdir = $(sysconfdir)/xdg/autostart
autostart_DATA = $(desktop_data)

paneldir = @PANEL_DIR@
panel_DATA = $(desktop_data)

servicedir = $(datadir)/dbus-1/services
service_in_files = org.moblin.UX.Shell.Panels.bluetooth.service.in
service_DATA = org.moblin.UX.Shell.Panels.bluetooth.service

@INTLTOOL_DESKTOP_RULE@

CLEANFILES = $(autostart_DATA)

EXTRA_DIST = $(autostart_in_in_files) $(ui_DATA) $(service_DATA)

MAINTAINERCLEANFILES = Makefile.in

DISTCLEANFILES = $(service_DATA)

%.service: %.service.in
	$(QUIET_GEN)sed -e "s|\@bindir\@|$(bindir)|" $< > $@
