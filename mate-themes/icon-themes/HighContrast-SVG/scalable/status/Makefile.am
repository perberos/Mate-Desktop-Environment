themedir =  $(datadir)/icons/HighContrast-SVG
size = scalable
context = status

icondir = $(themedir)/$(size)/$(context)

icon_DATA =				\
	audio-volume-high.svg \
	audio-volume-low.svg \
	audio-volume-medium.svg \
	audio-volume-muted.svg \
	battery-low.svg \
	battery-caution.svg \
	dialog-error.svg \
	dialog-information.svg \
	dialog-password.svg \
	dialog-question.svg \
	dialog-warning.svg \
	folder-drag-accept.svg \
	folder-open.svg \
	folder-visiting.svg \
	image-missing.svg \
	mail-attachment.svg \
	media-playlist-repeat.svg \
	media-playlist-shuffle.svg \
	network-offline.svg \
	network-online.svg \
	printer-printing.svg \
	software-update-available.svg \
	software-update-urgent.svg \
	user-trash-full.svg \
	weather-clear.svg \
	weather-clear-night.svg \
	weather-few-clouds.svg \
	weather-few-clouds-night.svg \
	weather-fog.svg \
	weather-overcast.svg \
	weather-showers.svg \
	weather-showers-scattered.svg \
	weather-snow.svg \
	weather-storm.svg

placeholder_FILES = \
	appointment-missed.svg \
	appointment-soon.svg \
	image-loading.svg \
	mail-read.svg \
	mail-replied.svg \
	mail-signed.svg \
	mail-signed-verified.svg \
	mail-unread.svg \
	network-error.svg \
	network-idle.svg \
	network-receive.svg \
	network-transmit.svg \
	network-transmit-receive.svg \
	security-high.svg \
	security-medium.svg \
	security-low.svg \
	sync-error.svg \
	sync-synchronizing.svg \
	task-due.svg \
	task-past-due.svg \
	user-away.svg \
	user-idle.svg \
	user-offline.svg \
	user-online.svg \
	weather-severe-alert.svg

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
