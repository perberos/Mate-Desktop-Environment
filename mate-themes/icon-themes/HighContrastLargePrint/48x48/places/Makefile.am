NULL=

themedir = $(datadir)/icons/HighContrastLargePrint
size = 48x48
context = places

icondir = $(themedir)/$(size)/$(context)

icon_DATA=	\
		folder-remote.png \
		folder.icon \
		folder.png \
		network-server.png \
		network-workgroup.png \
		start-here.png \
		user-desktop.png \
		user-home.png \
		user-trash.png \
		$(NULL)

EXTRA_DIST=	$(icon_DATA)

if LEGACY_ICON_MAPPING
install-data-local: install-iconDATA
	(cd $(DESTDIR)$(themedir)/$(size) && $(ICONMAP) -c $(context))
endif
