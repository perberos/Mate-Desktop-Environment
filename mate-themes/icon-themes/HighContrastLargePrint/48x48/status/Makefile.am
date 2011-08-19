NULL=

themedir = $(datadir)/icons/HighContrastLargePrint
size = 48x48
context = status

icondir = $(themedir)/$(size)/$(context)

icon_DATA=	\
		audio-volume-high.png \
		audio-volume-low.png \
		audio-volume-medium.png \
		audio-volume-muted.png \
		folder-drag-accept.icon \
		folder-drag-accept.png \
		network-error.png \
		network-idle.png \
		network-offline.png \
		network-receive.png \
		network-transmit-receive.png \
		network-transmit.png \
		user-trash-full.png \
		$(NULL)

EXTRA_DIST=	$(icon_DATA)

if LEGACY_ICON_MAPPING
install-data-local: install-iconDATA
	(cd $(DESTDIR)$(themedir)/$(size) && $(ICONMAP) -c $(context))
endif
