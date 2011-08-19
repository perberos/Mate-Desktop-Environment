NULL=

themedir = $(datadir)/icons/HighContrastLargePrintInverse
size = 48x48
context = status

icondir = $(themedir)/$(size)/$(context)

icon_DATA=	\
		audio-volume-high.png \
		audio-volume-low.png \
		audio-volume-medium.png \
		audio-volume-muted.png \
		battery-caution.png \
		battery-low.png \
		folder-drag-accept.icon \
		folder-drag-accept.png \
		user-trash-full.png \
		media-playlist-repeat.png \
		media-playlist-shuffle.png \
		network-error.png \
		network-idle.png \
		network-offline.png \
		network-receive.png \
		network-transmit.png \
		network-transmit-receive.png \
		printer-error.png \
		printer-printing.png \
		weather-clear-night.png \
		weather-clear.png \
		weather-few-clouds-night.png \
		weather-few-clouds.png \
		weather-fog.png \
		weather-overcast.png \
		weather-showers.png \
		weather-showers-scattered.png \
		weather-snow.png \
		weather-storm.png \
		$(NULL)

EXTRA_DIST=	$(icon_DATA)

if LEGACY_ICON_MAPPING
install-data-local: install-iconDATA
	(cd $(DESTDIR)$(themedir)/$(size) && $(ICONMAP) -c $(context))
endif
