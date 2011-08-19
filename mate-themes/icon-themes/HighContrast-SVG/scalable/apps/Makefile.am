themedir =  $(datadir)/icons/HighContrast-SVG
size = scalable
context = apps

icondir = $(themedir)/$(size)/$(context)

icon_DATA = \
	accessories-calculator.svg \
	accessories-character-map.svg \
	accessories-text-editor.svg \
	help-browser.svg \
	multimedia-volume-control.svg \
	preferences-desktop-accessibility.svg \
	preferences-desktop-font.svg \
	preferences-desktop-locale.svg \
	preferences-desktop-screensaver.svg \
	system-file-manager.svg \
	utilities-system-monitor.svg \
	utilities-terminal.svg

placeholder_FILE = \
	accessories-dictionary.svg \
	preferences-desktop-keyboard.svg \
	preferences-desktop-multimedia.svg \
	preferences-desktop-theme.svg \
	preferences-desktop-wallpaper.svg \
	system-software-install.svg \
	system-software-update.svg

if PLACEHOLDERS
icon_DATA += $(placeholder_FILES)
endif

EXTRA_DIST = \
	$(icon_DATA) \
	$(placeholder_FILES)

if LEGACY_ICON_MAPPING
install-data-local: install-iconDATA
	(cd $(DESTDIR)$(themedir)/$(size) && $(ICONMAP) -c $(context))
endif


uninstall-symlinks:
	list=`ls $(DESTDIR)$(themedir)/$(size)/$(context)`; \
	for p in $$list; do \
		if test -h $(DESTDIR)$(themedir)/$(size)/$(context)/$$p; then \
			rm -f $(DESTDIR)$(themedir)/$(size)/$(context)/$$p; \
		fi; \
	done

uninstall-am: uninstall-iconDATA uninstall-symlinks
