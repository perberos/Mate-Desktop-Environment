NULL=

themedir = $(datadir)/icons/HighContrastLargePrintInverse
size = 48x48
context = apps

icondir = $(themedir)/$(size)/$(context)

icon_DATA=	\
		accessories-calculator.png \
		accessories-character-map.png \
		accessories-dictionary.png \
		accessories-text-editor.png \
		advanced-directory.png \
		applets-screenshooter.png \
		ax-applet.png \
		brasero.png \
		charpick.png \
		clock.png \
		display-capplet.png \
		eog.png \
		evince.png \
		evolution.png \
		file-roller.png \
		firefox-icon.png \
		ghex.png \
		gimp.png \
		gkb.png \
		gtkam-camera.png \
		mate-application-generic.png \
		mate-audio2.png \
		mate-calc3.png \
		mate-ccmime.png \
		mate-ccscreensaver.png \
		mate-cd.png \
		mate-folder-generic.png \
		mate-grecord.png \
		mate-home.png \
		mate-info.png \
		mate-log.png \
		mate-logo-icon-transparent.png \
		mate-mailcheck.png \
		mate-mini-commander.png \
		mate-mixer-applet.png \
		mate-palm.png \
		mate-panel.png \
		mate-searchtool-animation-rest.png \
		mate-searchtool-animation.png \
		mate-settings-accessibility-keyboard.png \
		mate-settings-default-applications.png \
		mate-settings-ui-behavior.png \
		mate-starthere.png \
		mate-windows.png \
		mate-workspace.png \
		gok.png \
		help-browser.png \
		launcher-program.png \
		mozilla-icon.png \
		multimedia-volume-control.png \
		orca.png \
		pidgin.png \
		panel-drawer.png \
		panel-force-quit.png \
		perfmeter.png \
		preferences-desktop-accessibility.png \
		preferences-desktop-font.png \
		preferences-desktop-keyboard.png \
		preferences-desktop-keyboard-shortcuts.png \
		preferences-desktop-locale.png \
		preferences-desktop-sound.png \
		preferences-desktop-theme.png \
		preferences-desktop-wallpaper.png \
		preferences-system-session.png \
		preferences-system-windows.png \
		rhythmbox.png \
		smc.png \
		sound-juicer.png \
		star.png \
		system-file-manager.png \
		thunderbird-icon.png \
		totem.png \
		utilities-terminal.png \
		utilities-system-monitor.png \
		window-capplet.png \
		$(NULL)

EXTRA_DIST=	$(icon_DATA) \
		symlinks.dat

if LEGACY_ICON_MAPPING
install-data-local: install-iconDATA
	(cd $(DESTDIR)$(themedir)/$(size) && $(ICONMAP) -c $(context))
endif

install: install-am
	$(SHELL) $(top_srcdir)/common/mkiconlinks.sh $(srcdir)/symlinks.dat $(DESTDIR)$(icondir) 
 
uninstall-symlinks:
	list=`ls $(DESTDIR)$(icondir)`; for p in $$list; do \
		if test -h $(DESTDIR)$(icondir)/$$p; then \
			rm -f $(DESTDIR)$(icondir)/$$p; \
		fi; \
	done
uninstall-am: uninstall-iconDATA uninstall-symlinks
