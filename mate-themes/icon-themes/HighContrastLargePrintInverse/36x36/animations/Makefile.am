NULL=

themedir = $(datadir)/icons/HighContrastLargePrintInverse
size = 36x36
context = animations

icondir = $(themedir)/$(size)/$(context)

icon_DATA=	\
		process-idle.png \
		process-working.png \
		$(NULL)

EXTRA_DIST=	$(icon_DATA)

if LEGACY_ICON_MAPPING
install-data-local: install-iconDATA
	(cd $(DESTDIR)$(themedir)/$(size) && $(ICONMAP) -c $(context))
endif
