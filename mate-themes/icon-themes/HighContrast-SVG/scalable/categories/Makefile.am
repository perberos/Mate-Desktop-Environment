themedir =  $(datadir)/icons/HighContrast-SVG
size = scalable
context = categories

icondir = $(themedir)/$(size)/$(context)

icon_DATA = \
	applications-accessories.svg \
	applications-development.svg \
	applications-games.svg \
	applications-graphics.svg \
	applications-internet.svg \
	applications-multimedia.svg \
	applications-system.svg \
	preferences-desktop-peripherals.svg \
	preferences-desktop.svg \
	preferences-system.svg

placeholder_FILES = \
	applications-engineering.svg \
	applications-office.svg \
	applications-other.svg \
	applications-science.svg \
	applications-utilities.svg \
	preferences-desktop-personal.svg \
	preferences-other.svg \
	preferences-system-network.svg \
	system-help.svg

if PLACEHOLDERS
icon_DATA += $(placeholder_FILES)
endif

EXTRA_DIST = \
	$(icon_DATA)
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
