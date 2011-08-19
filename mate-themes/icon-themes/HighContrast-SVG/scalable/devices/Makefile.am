themedir =  $(datadir)/icons/HighContrast-SVG
size = scalable
context = devices

icondir = $(themedir)/$(size)/$(context)

icon_DATA = \
	audio-input-microphone.svg \
	battery.svg \
	camera-photo.svg \
	camera-video.svg \
	camera-web.svg \
	computer.svg \
	drive-harddisk.svg \
	drive-optical.svg \
	drive-removable-media.svg \
	input-keyboard.svg \
	input-mouse.svg \
	input-tablet.svg \
	media-flash.svg \
	media-floppy.svg \
	media-optical.svg \
	media-tape.svg \
	multimedia-player.svg \
	network-wired.svg \
	network-wireless.svg \
	pda.svg \
	phone.svg \
	printer.svg \
	scanner.svg \
	video-display.svg

placeholder_FILES = \
	audio-card.svg \
	input-gaming.svg \
	modem.svg

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
