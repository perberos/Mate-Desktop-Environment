NULL=

themedir = $(datadir)/icons/HighContrastLargePrintInverse
size = 48x48
context = actions

icondir = $(themedir)/$(size)/$(context)

icon_DATA=	\
		go-home.png \
		insert-image.png \
		insert-text.png \
		media-playback-pause.png \
		mail-send-receive.png \
		stock_new-tab.png \
		system-log-out.png \
		system-run.png \
		system-search.png \
		system-shutdown.png \
		view-refresh.png \
		$(NULL)

EXTRA_DIST=	$(icon_DATA) 

if LEGACY_ICON_MAPPING
install-data-local: install-iconDATA
	(cd $(DESTDIR)$(themedir)/$(size) && $(ICONMAP) -c $(context))
endif
