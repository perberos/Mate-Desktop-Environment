NULL =

SUBDIRS = icons

desktopdir = $(datadir)/applications
desktop_in_files = palimpsest.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

autostartdir = $(sysconfdir)/xdg/autostart
autostart_in_files = gdu-notification-daemon.desktop.in
autostart_DATA = $(autostart_in_files:.desktop.in=.desktop)

gdu-notification-daemon.desktop.in: gdu-notification-daemon.desktop.in.in
	@sed -e "s|\@LIBEXECDIR\@|$(libexecdir)|" $< > $@

@INTLTOOL_DESKTOP_RULE@

distuninstallcheck_listfiles = find . -type f -print | grep -v scrollkeeper

EXTRA_DIST = 			\
	$(desktop_in_files)	\
	$(autostart_in_files)	\
	$(NULL)

CLEANFILES = \
	$(desktop_DATA)				\
	$(autostart_DATA)			\
	gdu-notification-daemon.desktop.in	\
	gdu-notification-daemon.desktop.in.in	\
	$(NULL)

clean-local :
	rm -f *~
