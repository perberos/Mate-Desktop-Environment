NULL=

themedir = $(datadir)/icons/HighContrastLargePrint
size = 48x48
context = actions

icondir = $(themedir)/$(size)/$(context)

icon_DATA=	\
		go-home.png \
		media-playback-pause.png \
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
