bin_PROGRAMS = mate-notification-properties

@INTLTOOL_DESKTOP_RULE@

mate_notification_properties_LDADD   = $(NOTIFICATION_CAPPLET_LIBS)
mate_notification_properties_SOURCES = mate-notification-properties.c

uidir = $(datadir)/mate-notification-daemon
ui_DATA = mate-notification-properties.ui

desktopdir = $(datadir)/applications
desktop_in_files = mate-notification-properties.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

INCLUDES = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src/daemon \
	$(NOTIFICATION_CAPPLET_CFLAGS) \
	-DENGINES_DIR=\"$(libdir)/mate-notification-daemon/engines\" \
	-DNOTIFICATION_LOCALEDIR=\"$(datadir)/locale\" \
	-DNOTIFICATION_UIDIR=\"$(datadir)/mate-notification-daemon\"

CLEANFILES = $(desktop_DATA)
EXTRA_DIST = $(ui_DATA) $(pixmap_DATA) $(desktop_in_files)

-include $(top_srcdir)/git.mk
