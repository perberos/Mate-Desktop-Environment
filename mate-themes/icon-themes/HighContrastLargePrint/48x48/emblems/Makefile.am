NULL=

themedir = $(datadir)/icons/HighContrastLargePrint
size = 48x48
context = emblems

icondir = $(themedir)/$(size)/$(context)

icon_DATA=	\
		emblem-default.png \
		emblem-desktop.png \
		emblem-important.png \
		emblem-personal.png \
		emblem-special.png \
		emblem-unreadable.png \
		$(NULL)

EXTRA_DIST=	$(icon_DATA)

if LEGACY_ICON_MAPPING
install-data-local: install-iconDATA
	(cd $(DESTDIR)$(themedir)/$(size) && $(ICONMAP) -c $(context))
endif
