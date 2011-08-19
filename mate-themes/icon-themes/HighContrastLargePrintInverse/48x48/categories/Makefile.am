NULL=

themedir = $(datadir)/icons/HighContrastLargePrintInverse
size = 48x48
context = categories

icondir = $(themedir)/$(size)/$(context)

icon_DATA=	\
	applications-science.png \
	preferences-system-network.png \
	applications-development.png \
	applications-other.png \
	applications-utilities.png \
	preferences-desktop-personal.png \
	preferences-system.png \
	applications-system.png \
	applications-office.png \
	applications-engineering.png \
	applications-accessories.png \
	preferences-desktop-peripherals.png \
	applications-internet.png \
	applications-graphics.png \
	preferences-desktop.png \
	preferences-other.png \
	applications-games.png \
	applications-multimedia.png \
	$(NULL)

EXTRA_DIST=	$(icon_DATA) 

if LEGACY_ICON_MAPPING
install-data-local: install-iconDATA
	(cd $(DESTDIR)$(themedir)/$(size) && $(ICONMAP) -c $(context))
endif
